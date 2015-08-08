#include "Logger.h"
#include "EPoll.h"
#include "Network.h"
#include "ConnectionHandler.h"
#include "ProcessingPipeline.h"

ProcessingPipelinePacket::ProcessingPipelinePacket(ProcessingPipelinePacketType v_type) : type(v_type) {}

void ProcessingPipelinePacket::setPacketType(ProcessingPipelinePacketType v_type) {
    type = v_type;
}

void ProcessingPipelinePacket::setPacketRequestData(http::BufferedRequest* v_data) {
    request_data = v_data;
}

void ProcessingPipelinePacket::setPacketResponseData(http::BufferedResponse* v_data) {
    response_data = v_data;
}

void ProcessingPipelinePacket::setClientSocket(ClientSocket* socket) {
    client_socket = socket;
}

void ProcessingPipelinePacket::setFirstPipelineProcessor(FirstPipelineProcessor* v_processor) {
    first_processor = v_processor;
}

http::BufferedRequest* ProcessingPipelinePacket::getPacketRequestData() {
    return request_data;
}

http::BufferedResponse* ProcessingPipelinePacket::getPacketResponseData() {
    return response_data;
}

ProcessingPipelinePacketType ProcessingPipelinePacket::getPacketType() {
    return type;
}

ClientSocket* ProcessingPipelinePacket::getClientSocket() {
    return client_socket;
}

FirstPipelineProcessor* ProcessingPipelinePacket::getFirstPipelineProcessor() {
    return first_processor;
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
        void handleRead(char* read_buffer, int bytes_read, ClientSocket* socket) override {
            void* possible_packet = ClientSocketEventDataCommunicator::getEventData(socket);
            ProcessingPipelinePacket* packet;
            if (possible_packet == nullptr) {
                packet = new ProcessingPipelinePacket(PIPELINE_REQUEST);
                ClientSocketEventDataCommunicator::setEventData(socket, packet);
            } else {
                packet = static_cast<ProcessingPipelinePacket*>(possible_packet);
            }
            bool start_pipeline_process = connection_module->handleRead(packet, read_buffer, bytes_read);
            if (start_pipeline_process) {
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
    PipelineProcessor* currentProcessor = nullptr;
    bool is_response = false;
    if (data->getPacketType() == PIPELINE_REQUEST) {
        Logger::info << "Started request pipeline" << endl;
        data->setFirstPipelineProcessor(this);
        currentProcessor = backend_server;
    } else {
        Logger::info << "Started response pipeline" << endl;
        currentProcessor = nullptr;
        is_response = true;
    }

    while (currentProcessor != nullptr) {
        currentProcessor = currentProcessor->processAndGetNextProcessor(data);
    }

    if (is_response) {
        Logger::info << "Starting return to frontend server" << endl;
        frontend_server->processResponse(data->getClientSocket(), client_connection_module->handleWrite(data));
    }
}

ProcessingPipelineData * ProcessingPipeline::getProcessingPipelineData() {
    return pipeline_data;
}

void PipelineProcessor::setProcessingPipelineData(ProcessingPipelineData* pipeline_data) {
    processing_pipeline_data = pipeline_data;
}