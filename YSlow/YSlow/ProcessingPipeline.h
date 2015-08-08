#pragma once
#include "../httpxx/BufferedMessage.hpp"

class ClientSocket;
class FrontendServer;
class BackendServer;
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

class ProcessingPipelinePacket;

class PipelineProcessor {
    public:
        virtual void setProcessingPipelineData(ProcessingPipelineData* pipeline_data);
        virtual PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) = 0;
    protected:
        ProcessingPipelineData* processing_pipeline_data;
};

class RequestPipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) override;
        virtual PipelineProcessor* processRequest(ProcessingPipelinePacket* data) = 0;
};

class ResponsePipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) override;
        virtual PipelineProcessor* processResponse(ProcessingPipelinePacket* data) = 0;
};

class ClientConnectionModule;

class FirstPipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) override;
        virtual void startPipelineProcess(ProcessingPipelinePacket* data) = 0;
};

class ProcessingPipelinePacket {
    public:
        ProcessingPipelinePacket(ProcessingPipelinePacketType v_type);
        void setPacketType(ProcessingPipelinePacketType v_type);
        void setPacketRequestData(http::BufferedRequest* v_data);
        void setPacketResponseData(http::BufferedResponse* v_data);
        void setClientSocket(ClientSocket* socket);
        void setFirstPipelineProcessor(FirstPipelineProcessor* v_processor);
        http::BufferedRequest* getPacketRequestData();
        http::BufferedResponse* getPacketResponseData();
        ProcessingPipelinePacketType getPacketType();
        ClientSocket* getClientSocket();
        FirstPipelineProcessor* getFirstPipelineProcessor();
    private:
        ProcessingPipelinePacketType type;
        ClientSocket* client_socket = nullptr;
        FirstPipelineProcessor* first_processor = nullptr;
        http::BufferedRequest* request_data = nullptr;
        http::BufferedResponse* response_data = nullptr;
};

class InitialClientReadHandler;

class ProcessingPipeline : FirstPipelineProcessor {
    public:
        ProcessingPipeline(ProcessingPipelineData* v_pipeline_data);
        ~ProcessingPipeline();
        void setupPipeline();
        void startPipelineProcess(ProcessingPipelinePacket* data) override;
        ProcessingPipelineData* getProcessingPipelineData();
    private:
        ProcessingPipelineData* pipeline_data;
        InitialClientReadHandler* client_read_handler;
        FrontendServer* frontend_server;
        BackendServer* backend_server;
        ClientConnectionModule* client_connection_module;
};
