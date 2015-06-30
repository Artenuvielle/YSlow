#include "Logger.h"
#include "EPoll.h"
#include "Network.h"
#include "ConnectionHandler.h"
#include "ProcessingPipeline.h"

ProcessingPipelinePacket::ProcessingPipelinePacket(ProcessingPipelinePacketType v_type) : type(v_type) {}

void ProcessingPipelinePacket::setPacketType(ProcessingPipelinePacketType v_type) {
    type = v_type;
}

void ProcessingPipelinePacket::setPacketData(char * v_data, int v_length) {
    data = v_data;
    length = v_length;
}

void ProcessingPipelinePacket::setClientSocket(ClientSocket* socket) {
    client_socket = socket;
}

char* ProcessingPipelinePacket::getPacketData() {
    return data;
}

int ProcessingPipelinePacket::getPacketDataLength() {
    return length;
}

ProcessingPipelinePacketType ProcessingPipelinePacket::getPacketType() {
    return type;
}

ClientSocket * ProcessingPipelinePacket::getClientSocket() {
    return client_socket;
}

PipelineProcessor* RequestPipelineProcessor::processAndGetNextProcessor(ProcessingPipelinePacket* data) {
    return processRequest(data);
}

PipelineProcessor* ResponsePipelineProcessor::processAndGetNextProcessor(ProcessingPipelinePacket* data) {
    return processResponse(data);
}

PipelineProcessor* FirstPipelineProcessor::processAndGetNextProcessor(ProcessingPipelinePacket* data) {
    startPipelineProcess(data);
    return nullptr;
}

class InitialClientReadHandler : public ClientSocketReadHandler {
    public:
        InitialClientReadHandler(FirstPipelineProcessor* v_first_processor) {
            first_processor = v_first_processor;
        }
        void setConnectionModule(ClientConnectionModule* v_connection_module) {
            connection_module = v_connection_module;
        }
        void handleRead(char* read_buffer, int bytes_read, ClientSocket* socket) {
            ProcessingPipelinePacket* packet = connection_module->handleRead(read_buffer, bytes_read);
            if (packet != nullptr) {
                packet->setClientSocket(socket);
                first_processor->startPipelineProcess(packet);
            }
        }
    private:
        ClientConnectionModule* connection_module;
        FirstPipelineProcessor* first_processor;
};

ProcessingPipeline::ProcessingPipeline(ProcessingPipelineData* v_pipeline_data) : pipeline_data(v_pipeline_data) {
    frontend_server = new FrontendServer(pipeline_data);
    backend_server = new BackendServer(pipeline_data);
    client_connection_module = new HTTPClientConnectionModule(pipeline_data);
    client_read_handler = new InitialClientReadHandler(this);
    client_read_handler->setConnectionModule(client_connection_module);
    frontend_server->setClientConnectionHandler(client_read_handler);
}

ProcessingPipeline::~ProcessingPipeline() {
    delete pipeline_data->epoll_manager;
    delete client_connection_module;
    delete client_read_handler;
    delete frontend_server;
    delete backend_server;
}

void ProcessingPipeline::setupPipeline() {
    Logger::info << "Started.  Listening on port " << pipeline_data->frontend_server_port_string << "." << endl;
    pipeline_data->epoll_manager->executePollingLoop();
}

void ProcessingPipeline::startPipelineProcess(ProcessingPipelinePacket* data) {
    backend_server->processRequest(data);
}

ProcessingPipelineData * ProcessingPipeline::getProcessingPipelineData() {
    return pipeline_data;
}

void PipelineProcessor::setProcessingPipelineData(ProcessingPipelineData* pipeline_data) {
    processing_pipeline_data = pipeline_data;
}