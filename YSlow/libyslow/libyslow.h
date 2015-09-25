#pragma once
#include <map>
#include "../httpxx/BufferedMessage.hpp"

class EPollManager;
class PipelineProcessor;
class ClientSocket;
class PipelineProcessor;
class ClientConnectionModule;

using namespace std;

struct ProcessingPipelineData {
    EPollManager* epoll_manager;
    string frontend_server_port_string;
    int first_pipeline_processor_index;
    map<int, PipelineProcessor*>* pipeline_processors;
    ClientConnectionModule* connection_module;
};

enum ProcessingPipelinePacketType {
    PIPELINE_REQUEST,
    PIPELINE_RESPONSE
};

class ProcessingPipelinePacket;

class InitializationData {
    public:
        void setString(string v_data);
        void setInt(int v_data);
        void setMap(map<string, InitializationData*>* v_data);
        void setProcessor(PipelineProcessor* v_data);
        bool isString();
        bool isInt();
        bool isMap();
        bool isProcessor();
        string getString();
        int getInt();
        map<string, InitializationData*>* getMap();
        PipelineProcessor* getProcessor();
    private:
        enum InitializationDataType {
            TYPE_STRING,
            TYPE_INT,
            TYPE_MAP,
            TYPE_PROCESSOR
        };
        InitializationDataType data_type;
        void* data;
};

class PipelineProcessor {
    public:
        virtual void setProcessingPipelineData(ProcessingPipelineData* pipeline_data);
        virtual PipelineProcessor* processAndGetNextProcessor(ProcessingPipelinePacket* data) = 0;
        virtual void setInitializationData(string name, InitializationData* data) = 0;
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

class FirstPipelineProcessor;

class ProcessingPipelinePacket {
    public:
        ProcessingPipelinePacket(ProcessingPipelinePacketType v_type);
        void setPacketType(ProcessingPipelinePacketType v_type);
        void setPacketRequestData(http::BufferedRequest* v_data);
        void setPacketResponseData(http::BufferedResponse* v_data);
        void setClientSocket(ClientSocket* socket);
        void setFirstPipelineProcessor(FirstPipelineProcessor* v_processor);
        void setFirstResponsePipelineProcessor(ResponsePipelineProcessor* v_processor);
        http::BufferedRequest* getPacketRequestData();
        http::BufferedResponse* getPacketResponseData();
        ProcessingPipelinePacketType getPacketType();
        ClientSocket* getClientSocket();
        FirstPipelineProcessor* getFirstPipelineProcessor();
        PipelineProcessor* getFirstResponsePipelineProcessor();
    private:
        ProcessingPipelinePacketType type;
        ClientSocket* client_socket = nullptr;
        FirstPipelineProcessor* first_processor = nullptr;
        ResponsePipelineProcessor* first_response_processor = nullptr;
        http::BufferedRequest* request_data = nullptr;
        http::BufferedResponse* response_data = nullptr;
};

class ClientConnectionModule {
    public:
        ClientConnectionModule();
        void setPipelineData(ProcessingPipelineData* v_pipeline_data);
        virtual bool handleRead(ProcessingPipelinePacket* packet, char* read_buffer, int bytes_read) = 0;
        virtual string handleWrite(ProcessingPipelinePacket* packet) = 0;
    protected:
        ProcessingPipelineData* getPipelineData();
    private:
        ProcessingPipelineData* pipeline_data;
};

// typedef to make it easier to set up our factory
typedef PipelineProcessor* module_maker_t();
typedef ClientConnectionModule* connection_maker_t();
// our global factory
extern map<string, module_maker_t*, less<string> > module_library;
extern map<string, connection_maker_t*, less<string> > connection_library;