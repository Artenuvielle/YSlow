#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>

#include "epollinterface.h"
#include "connection.h"
#include "netutils.h"
#include "Logger.h"
#include "server_socket.h"

#define MAX_LISTEN_BACKLOG 4096

using namespace std;

struct server_socket_event_data {
	char* backend_addr;
	char* backend_port_str;
};

struct proxy_data {
	epoll_event_handler* client;
	epoll_event_handler* backend;
};

void on_client_read(void* closure, char* buffer, int len) {
	proxy_data* data = (proxy_data*) closure;
	if (data->backend == NULL) {
		return;
	}
	connection_write(data->backend, buffer, len);
}


void on_client_close(void* closure) {
	proxy_data* data = (proxy_data*) closure;
	if (data->backend == NULL) {
		return;
	}
	connection_close(data->backend);
	data->client = NULL;
	data->backend = NULL;
	epoll_add_to_free_list(closure);
}


void on_backend_read(void* closure, char* buffer, int len) {
	proxy_data* data = (proxy_data*) closure;
	if (data->client == NULL) {
		return;
	}
	connection_write(data->client, buffer, len);
}


void on_backend_close(void* closure) {
	proxy_data* data = (proxy_data*) closure;
	if (data->client == NULL) {
		return;
	}
	connection_close(data->client);
	data->client = NULL;
	data->backend = NULL;
	epoll_add_to_free_list(closure);
}

void handle_client_connection(int client_socket_fd, char* backend_host, char* backend_port_str) {
	epoll_event_handler* client_connection;
	Logger::debug << "Creating connection object for incoming connection..." << endl;
	client_connection = create_connection(client_socket_fd);

	int backend_socket_fd = connect_to_backend(backend_host, backend_port_str);
	epoll_event_handler* backend_connection;
	Logger::debug << "Creating connection object for backend connection..." << endl;
	backend_connection = create_connection(backend_socket_fd);

	proxy_data* proxy = new proxy_data;
	proxy->client = client_connection;
	proxy->backend = backend_connection;

	connection_closure* client_closure = (connection_closure*) client_connection->closure;
	client_closure->on_read = on_client_read;
	client_closure->on_read_closure = proxy;
	client_closure->on_close = on_client_close;
	client_closure->on_close_closure = proxy;

	connection_closure* backend_closure = (connection_closure*) backend_connection->closure;
	backend_closure->on_read = on_backend_read;
	backend_closure->on_read_closure = proxy;
	backend_closure->on_close = on_backend_close;
	backend_closure->on_close_closure = proxy;
}



void handle_server_socket_event(epoll_event_handler* self, uint32_t events) {
	server_socket_event_data* closure = (server_socket_event_data*) self->closure;

	int client_socket_fd;
	while (1) {
		client_socket_fd = accept(self->fd, NULL, NULL);
		if (client_socket_fd == -1) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				break;
			} else {
				Logger::error << "Could not accept" << endl;
				exit(1);
			}
		}

		handle_client_connection(client_socket_fd,
			closure->backend_addr,
			closure->backend_port_str);
	}
}


int create_and_bind(char* server_port_str) {
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

	int server_socket_fd;
	addrinfo* addr_iter;
	for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next) {
		server_socket_fd = socket(addr_iter->ai_family,
			addr_iter->ai_socktype,
			addr_iter->ai_protocol);
		if (server_socket_fd == -1) {
			continue;
		}

		int so_reuseaddr = 1;
		if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr)) != 0) {
			continue;
		}

		if (bind(server_socket_fd,
			addr_iter->ai_addr,
			addr_iter->ai_addrlen) == 0)
		{
			break;
		}

		close(server_socket_fd);
	}

	if (addr_iter == NULL) {
		Logger::info << "Couldn't bind" << endl;
		exit(1);
	}

	freeaddrinfo(addrs);

	return server_socket_fd;
}


epoll_event_handler* create_server_socket_handler(char* server_port_str, char* backend_addr, char* backend_port_str) {
	int server_socket_fd;
	server_socket_fd = create_and_bind(server_port_str);
	make_socket_non_blocking(server_socket_fd);

	listen(server_socket_fd, MAX_LISTEN_BACKLOG);

	server_socket_event_data* closure = new server_socket_event_data;
	closure->backend_addr = backend_addr;
	closure->backend_port_str = backend_port_str;

	epoll_event_handler* result = new epoll_event_handler;
	result->fd = server_socket_fd;
	result->handle = handle_server_socket_event;
	result->closure = closure;

	epoll_add_handler(result, EPOLLIN | EPOLLET);

	return result;
}
