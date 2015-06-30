#pragma once

class ProcessingPipelineData;

class ClientConnectionModule {
    public:
        ClientConnectionModule(ProcessingPipelineData* v_pipeline_data);
        virtual ProcessingPipelinePacket* handleRead(char* read_buffer, int bytes_read) = 0;
        virtual string handleWrite(ProcessingPipelinePacket* packet) = 0;
    protected:
        ProcessingPipelineData* getPipelineData();
    private:
        ProcessingPipelineData* pipeline_data;
};

class HTTPClientConnectionModule : public ClientConnectionModule {
    public:
        HTTPClientConnectionModule(ProcessingPipelineData* pipeline);
        ~HTTPClientConnectionModule();
        ProcessingPipelinePacket* handleRead(char* read_buffer, int bytes_read);
        string handleWrite(ProcessingPipelinePacket* packet);
};
