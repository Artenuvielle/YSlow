#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <fcntl.h>

#include "EPoll.h"
#include "Logger.h"
#include "ProcessingPipeline.h"
#include "Network.h"

#define MAX_LISTEN_BACKLOG 4096
#define BUFFER_SIZE 4096

using namespace std;

class Socket {
    public:
        int getSocketFileDescriptor() {
            return socket_file_descriptor;
        }
    protected:
        void setSocketFileDescriptor(int file_descriptor) {
            socket_file_descriptor = file_descriptor;
        }
        void makeSocketNonBlocking() {
            int flags;

            // get current flags
            flags = fcntl(socket_file_descriptor, F_GETFL, 0);
            if (flags == -1) {
                Logger::error << "Couldn't get socket flags" << endl;
                exit(1);
            }

            // extend flags with non-blocking-flag
            flags |= O_NONBLOCK;

            // set new flags
            if (fcntl(socket_file_descriptor, F_SETFL, flags) == -1) {
                Logger::error << "Couldn't set socket flags" << endl;
                exit(-1);
            }
        }
    private:
        int socket_file_descriptor;
};

class ServerSocket : public Socket {
    public:
        ServerSocket(char* server_port_str) {
            initServerSocket(server_port_str);
            makeSocketNonBlocking();
        }
    private:
        void initServerSocket(char* server_port_str) {
            addrinfo hints;
            memset(&hints, 0, sizeof(addrinfo));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;

            addrinfo* addrs;
            int getaddrinfo_error;
            getaddrinfo_error = getaddrinfo(NULL, server_port_str, &hints, &addrs);
            if (getaddrinfo_error != 0) {
                Logger::info << "Couldn't find local host details: " << gai_strerror(getaddrinfo_error) << endl;
                exit(1);
            }

            addrinfo* addr_iterator;
            int temporary_socket_file_descriptor;
            for (addr_iterator = addrs; addr_iterator != NULL; addr_iterator = addr_iterator->ai_next) {
                temporary_socket_file_descriptor = socket(addr_iterator->ai_family, addr_iterator->ai_socktype, addr_iterator->ai_protocol);
                if (temporary_socket_file_descriptor == -1) {
                    continue;
                }

                int so_reuse_addr = 1;
                if (setsockopt(temporary_socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &so_reuse_addr, sizeof(so_reuse_addr)) != 0) {
                    continue;
                }

                if (bind(temporary_socket_file_descriptor, addr_iterator->ai_addr, addr_iterator->ai_addrlen) == 0) {
                    setSocketFileDescriptor(temporary_socket_file_descriptor);
                    break;
                }

                close(temporary_socket_file_descriptor);
            }

            if (addr_iterator == NULL) {
                Logger::info << "Couldn't bind" << endl;
                exit(1);
            }

            freeaddrinfo(addrs);
        }
};

struct DataBufferEntry {
    int is_close_message;
    char* data;
    int current_offset;
    int len;
    DataBufferEntry* next;
};

class ClientSocket : public Socket, EPollEventHandler {
    public:
        ClientSocket(int file_descriptor, EPollManager* v_epoll_manager, ClientSocketReadHandler* v_read_handler = nullptr, ClientSocketCloseHandler* v_close_handler = nullptr) : EPollEventHandler(file_descriptor) {
            setSocketFileDescriptor(file_descriptor);
            epoll_manager = v_epoll_manager;
            read_handler = v_read_handler;
            close_handler = v_close_handler;
            makeSocketNonBlocking();
            epoll_manager->addEventHandler(this, EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLOUT);
        }
        ~ClientSocket() {
            DataBufferEntry* next;
            while (write_buffer != nullptr) {
                next = write_buffer->next;
                if (!write_buffer->is_close_message) {
                    free(write_buffer->data);
                }
                delete write_buffer;
                write_buffer = next;
            }
            epoll_manager->removeEventHandler(this);
        }
        void handleEPollEvent(uint32_t events) {
            if (events & EPOLLOUT) {
                onOutEvent();
            }

            if (events & EPOLLIN) {
                onInEvent();
            }

            if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
                onCloseEvent();
            }
        }
        void send(char* data, int length) {
            int written = 0;
            if (write_buffer == nullptr) {
                written = write(getSocketFileDescriptor(), data, length);
                if (written == length) {
                    return;
                }
            }
            if (written == -1) {
                if (errno == ECONNRESET || errno == EPIPE) {
                    Logger::error << "Connection write error" << endl;
                    onCloseEvent();
                    return;
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    Logger::error << "Error writing to client" << endl;
                    exit(-1);
                }
                written = 0;
            }

            int unwritten = length - written;
            DataBufferEntry* new_entry = new DataBufferEntry;
            new_entry->is_close_message = 0;
            new_entry->data = (char*)malloc(unwritten);
            memcpy(new_entry->data, data + written, unwritten);
            new_entry->current_offset = 0;
            new_entry->len = unwritten;
            new_entry->next = nullptr;
            addWriteBufferEntry(new_entry);
        }
        void requestClose() {
            onCloseEvent();
        }
        void setEventData(void* v_event_data) {
            event_data = v_event_data;
        }
        void* getEventData() {
            return event_data;
        }
    private:
        void* event_data;
        EPollManager* epoll_manager;
        ClientSocketReadHandler* read_handler = nullptr;
        ClientSocketCloseHandler* close_handler = nullptr;
        DataBufferEntry* write_buffer = nullptr;
        void onOutEvent() {
            int written;
            int to_write;
            DataBufferEntry* temp;
            while (write_buffer != NULL) {
                if (write_buffer->is_close_message) {
                    closeConnection();
                    return;
                }

                to_write = write_buffer->len - write_buffer->current_offset;
                written = write(getSocketFileDescriptor(), write_buffer->data + write_buffer->current_offset, to_write);
                if (written != to_write) {
                    if (written == -1) {
                        if (errno == ECONNRESET || errno == EPIPE) {
                            Logger::error << "On out event write error" << endl;
                            onCloseEvent();
                            return;
                        }
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            Logger::error << "Error writing to client" << endl;
                            exit(-1);
                        }
                        written = 0;
                    }
                    write_buffer->current_offset += written;
                    break;
                } else {
                    temp = write_buffer;
                    write_buffer = write_buffer->next;
                    free(temp->data);
                    delete temp;
                }
            }
        }
        void onInEvent() {
            char read_buffer[BUFFER_SIZE];
            int bytes_read;

            while ((bytes_read = read(getFileDescriptor(), read_buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) {
                if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    return;
                }

                if (bytes_read == 0 || bytes_read == -1) {
                    onCloseEvent();
                    return;
                }

                if (read_handler != nullptr) {
                    read_handler->handleRead(read_buffer, bytes_read, this);
                }
            }
        }
        void onCloseEvent() {
            if (write_buffer == nullptr) {
                closeConnection();
            } else {
                DataBufferEntry* new_entry = new DataBufferEntry;
                new_entry->is_close_message = 1;
                new_entry->next = nullptr;
                addWriteBufferEntry(new_entry);
            }
        }
        void addWriteBufferEntry(DataBufferEntry* new_entry) {
            if (write_buffer == nullptr) {
                write_buffer = new_entry;
            } else {
                DataBufferEntry* last_buffer_entry;
                for (last_buffer_entry = write_buffer; last_buffer_entry->next != nullptr; last_buffer_entry = last_buffer_entry->next) {
                }
                last_buffer_entry->next = new_entry;
            }
        }
        void closeConnection() {
            close(getSocketFileDescriptor());
            if (close_handler != nullptr) {
                close_handler->handleClose(this);
            }
        }
};

class FrontendEPollHandler : public EPollEventHandler {
    public:
        FrontendEPollHandler(int file_descriptor, NewConnectionHandler* v_connection_handler) : EPollEventHandler(file_descriptor) {
            connection_handler = v_connection_handler;
        }
        void handleEPollEvent(uint32_t events) {
            int client_socket_fd;
            while (1) {
                client_socket_fd = accept(getFileDescriptor(), NULL, NULL);
                if (client_socket_fd == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        break;
                    } else {
                        Logger::error << "Could not accept" << endl;
                        exit(1);
                    }
                }
                connection_handler->handleNewConnection(client_socket_fd);
            }
        }
    private:
        NewConnectionHandler* connection_handler;
};

FrontendServer::FrontendServer(ProcessingPipelineData* pipeline_data) {
    ServerSocket* socket = new ServerSocket(pipeline_data->frontend_server_port_string);

    listen(socket->getSocketFileDescriptor(), MAX_LISTEN_BACKLOG);

    epoll_manager = pipeline_data->epoll_manager;
    response_handler = nullptr;

    event_handler = new FrontendEPollHandler(socket->getSocketFileDescriptor(), this);

    pipeline_data->epoll_manager->addEventHandler(event_handler, EPOLLIN | EPOLLET);
}

FrontendServer::~FrontendServer() {
}

void FrontendServer::setClientConnectionHandler(ClientSocketReadHandler* connection_module) {
    response_handler = connection_module;
}

void FrontendServer::handleClose(ClientSocket* socket) {
    ClientSocketList* previous_client = nullptr;
    ClientSocketList* this_client;
    for (this_client = client_list; this_client->socket != socket; this_client = this_client->next_socket) {
        if (this_client->next_socket == nullptr) {
            Logger::warn << "Could not find client socket to close" << endl;
            delete socket;
            return;
        }
        previous_client = this_client;
    }
    if (previous_client != nullptr) {
        previous_client->next_socket = this_client->next_socket;
    }
    delete socket;
    delete this_client;
    Logger::info << "Frontend connection closed" << endl;
}

void FrontendServer::handleNewConnection(int file_descriptor) {
    Logger::info << "Creating connection object for incoming connection..." << endl;
    ClientSocket* client_socket = new ClientSocket(file_descriptor, epoll_manager, response_handler, this);
    ClientSocketList* new_client_socket_list_element = new ClientSocketList;
    *new_client_socket_list_element = { client_socket, client_list };
    client_list = new_client_socket_list_element;
}

class BackendResponseHandler : public ClientSocketReadHandler, public ClientSocketCloseHandler {
    public:
        void handleRead(char* read_buffer, int bytes_read, ClientSocket* socket) {
            Logger::info << "Got response from backend (" << bytes_read << " byte): " << read_buffer << endl;
            ((ClientSocket*)socket->getEventData())->send(read_buffer, bytes_read);
            //socket->requestClose();
            // TODO: Evaluat whether to close this connection later
            //((ClientSocket*)socket->getEventData())->requestClose();
        }
        void handleClose(ClientSocket* socket) {
            Logger::info << "Backend connection closed" << endl;
        }
};

BackendServer::BackendServer(ProcessingPipelineData* pipeline_data) {
    epoll_manager = pipeline_data->epoll_manager;
    backend_server_host_string = pipeline_data->backend_server_host_string;
    backend_server_port_string = pipeline_data->backend_server_port_string;
    response_handler = new BackendResponseHandler();
}

BackendServer::~BackendServer() {
}

PipelineProcessor* BackendServer::processRequest(ProcessingPipelinePacket* data) {
    int backend_socket_file_descriptor = initBackendConnection();
    ClientSocket* socket = new ClientSocket(backend_socket_file_descriptor, epoll_manager, response_handler, response_handler);
    socket->setEventData(data->getClientSocket());
    socket->send(data->getPacketData(), data->getPacketDataLength());

    ClientSocketList* new_client_socket_list_element = new ClientSocketList;
    *new_client_socket_list_element = { socket, client_list };
    client_list = new_client_socket_list_element;
}

int BackendServer::initBackendConnection() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int getaddrinfo_error;
    struct addrinfo* addrs;
    getaddrinfo_error = getaddrinfo(backend_server_host_string, backend_server_port_string, &hints, &addrs);
    if (getaddrinfo_error != 0) {
        if (getaddrinfo_error == EAI_SYSTEM) {
            Logger::error << "Couldn't find backend " << backend_server_host_string << ":" << backend_server_port_string << endl;
        } else {
            Logger::error << "Couldn't find backend " << backend_server_host_string << ":" << backend_server_port_string << " : " << gai_strerror(getaddrinfo_error) << endl;
        }
        exit(1);
    }

    struct addrinfo* addrs_iter;
    int backend_socket_file_descriptor;
    for (addrs_iter = addrs; addrs_iter != NULL; addrs_iter = addrs_iter->ai_next) {
        backend_socket_file_descriptor = socket(addrs_iter->ai_family, addrs_iter->ai_socktype, addrs_iter->ai_protocol);
        if (backend_socket_file_descriptor == -1) {
            continue;
        }

        if (connect(backend_socket_file_descriptor, addrs_iter->ai_addr, addrs_iter->ai_addrlen) != -1) {
            break;
        }

        close(backend_socket_file_descriptor);
    }

    if (addrs_iter == NULL) {
        Logger::error << "Couldn't connect to backend" << endl;
        exit(1);
    }

    freeaddrinfo(addrs);

    return backend_socket_file_descriptor;
}