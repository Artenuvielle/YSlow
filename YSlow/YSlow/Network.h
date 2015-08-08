#pragma once

#include "ProcessingPipeline.h"

struct ProcessingPipelineData;
class FrontendEPollHandler;
class EPollManager;
class BackendResponseHandler;
class ClientSocket;

class ClientSocketEventDataCommunicator {
    public:
        static void setEventData(ClientSocket* socket, void* data);
        static void* getEventData(ClientSocket* socket);
    private:
        ClientSocketEventDataCommunicator() {}
};

class ClientSocketReadHandler {
    public:
        virtual void handleRead(char* read_buffer, int bytes_read, ClientSocket* socket) = 0;
};

class ClientSocketCloseHandler {
    public:
        virtual void handleClose(ClientSocket* socket) = 0;
};

class NewConnectionHandler {
    public:
        virtual void handleNewConnection(int file_descriptor) = 0;
};

struct ClientSocketList {
    ClientSocket* socket;
    ClientSocketList* next_socket;
};

class FrontendServer : NewConnectionHandler, ClientSocketCloseHandler {
    public:
        FrontendServer(ProcessingPipelineData* pipeline_data);
        ~FrontendServer();
        void setClientConnectionHandler(ClientSocketReadHandler* handler);
        void handleNewConnection(int file_descriptor) override;
        void handleClose(ClientSocket* socket) override;
        void processResponse(ClientSocket* socket, string packet);
    private:
        EPollManager* epoll_manager;
        ClientSocketReadHandler* response_handler = nullptr;
        ClientSocketList* client_list = nullptr;
        FrontendEPollHandler* event_handler;
};

class BackendServer : public RequestPipelineProcessor {
    public:
        BackendServer(ProcessingPipelineData* pipeline_data);
        ~BackendServer();
        PipelineProcessor* processRequest(ProcessingPipelinePacket* data) override;
    private:
        char* backend_server_host_string;
        char* backend_server_port_string;
        EPollManager* epoll_manager;
        BackendResponseHandler* response_handler = nullptr;
        ClientSocketList* client_list = nullptr;
        int initBackendConnection();
};