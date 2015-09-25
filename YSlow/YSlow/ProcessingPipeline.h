#pragma once
#include "../libyslow/libyslow.h"

class ClientSocket;
class FrontendServer;
class BackendServer;

class ClientConnectionModule;

class FirstPipelineProcessor : public PipelineProcessor {
    public:
        PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) override;
        virtual void startPipelineProcess(ProcessingPipelinePacket* data) = 0;
};

class InitialClientReadHandler;

class ProcessingPipeline : FirstPipelineProcessor {
    public:
        ProcessingPipeline(ProcessingPipelineData* v_pipeline_data);
        ~ProcessingPipeline();
        void setupPipeline();
        void startPipelineProcess(ProcessingPipelinePacket* data) override;
        void setInitializationData(string name, InitializationData* data) override;
        ProcessingPipelineData* getProcessingPipelineData();
    private:
        ProcessingPipelineData* pipeline_data;
        InitialClientReadHandler* client_read_handler;
        FrontendServer* frontend_server;
        //BackendServer* backend_server;
        ClientConnectionModule* client_connection_module;
};
