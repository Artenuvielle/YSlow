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
        FrontendEPollHandler* event_handler;
};

class BackendServer : public RequestPipelineProcessor {
    public:
        BackendServer(EPollManager* v_epoll_manager, const char* v_backend_server_host_string, const char* v_backend_server_port_string);
        ~BackendServer();
        PipelineProcessor* processRequest(ProcessingPipelinePacket* data) override;
        void setInitializationData(string name, InitializationData* data) override;

    private:
        string backend_server_host_string;
        string backend_server_port_string;
        EPollManager* epoll_manager;
        BackendResponseHandler* response_handler = nullptr;
        int initBackendConnection();
};