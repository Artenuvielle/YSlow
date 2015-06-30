#pragma once

class FrontendServer;
class BackendServer;
class ClientSocket;
class EPollManager;

struct ProcessingPipelineData {
    EPollManager* epoll_manager;
    char* frontend_server_port_string;
    char* backend_server_host_string;
    char* backend_server_port_string;
};

enum ProcessingPipelinePacketType {
    PIPELINE_REQUEST,
    PIPELINE_RESPONSE
};

class ProcessingPipelinePacket {
    public:
        ProcessingPipelinePacket(ProcessingPipelinePacketType v_type);
        void setPacketType(ProcessingPipelinePacketType v_type);
        void setPacketData(char* v_data, int v_length);
        void setClientSocket(ClientSocket* socket);
        char* getPacketData();
        int getPacketDataLength();
        ProcessingPipelinePacketType getPacketType();
        ClientSocket* getClientSocket();
    private:
        ProcessingPipelinePacketType type;
        ClientSocket* client_socket;
        char* data;
        int length;
};

class PipelineProcessor {
    public:
        virtual void setProcessingPipelineData(ProcessingPipelineData* pipeline_data);
        virtual PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) = 0;
    protected:
        ProcessingPipelineData* processing_pipeline_data;
};

class RequestPipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data);
        virtual PipelineProcessor* processRequest(ProcessingPipelinePacket* data) = 0;
};

class ResponsePipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data);
        virtual PipelineProcessor* processResponse(ProcessingPipelinePacket* data) = 0;
};

class ClientConnectionModule;

class FirstPipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data);
        virtual void startPipelineProcess(ProcessingPipelinePacket* data) = 0;
};

class InitialClientReadHandler;

class ProcessingPipeline : FirstPipelineProcessor {
    public:
        ProcessingPipeline(ProcessingPipelineData* v_pipeline_data);
        ~ProcessingPipeline();
        void setupPipeline();
        void startPipelineProcess(ProcessingPipelinePacket* data);
        ProcessingPipelineData* getProcessingPipelineData();
    private:
        ProcessingPipelineData* pipeline_data;
        InitialClientReadHandler* client_read_handler;
        FrontendServer* frontend_server;
        BackendServer* backend_server;
        ClientConnectionModule* client_connection_module;
};
