#include "../libyslow/libyslow.h"
#include "../libyslow/Logger.h"
#include "EPoll.h"
#include "Network.h"
#include "ProcessingPipeline.h"

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
            ConnectionModuleDataCarrier* connection_data_carrier = ClientSocketCommunicator::getConnectionModuleDataCarrier(socket);
            connection_module->handleRead(connection_data_carrier, read_buffer, bytes_read);

            ProcessingPipelinePacket* packet = connection_module->getCompletePacket(connection_data_carrier);
            while (packet != nullptr) {
                packet->setClientSocket(socket);
                first_processor->startPipelineProcess(packet);
                packet = connection_module->getCompletePacket(connection_data_carrier);
            }
        }
    private:
        ClientConnectionModule* connection_module;
        FirstPipelineProcessor* first_processor;
};

ProcessingPipeline::ProcessingPipeline(ProcessingPipelineData* v_pipeline_data) : pipeline_data(v_pipeline_data) {
    frontend_server = new FrontendServer(pipeline_data);
    client_connection_module = pipeline_data->connection_module;
    client_read_handler = new InitialClientReadHandler(this);
    client_read_handler->setConnectionModule(client_connection_module);
    frontend_server->setClientConnectionHandler(client_read_handler);
}

ProcessingPipeline::~ProcessingPipeline() {
    delete pipeline_data->epoll_manager;
    delete client_connection_module;
    delete client_read_handler;
    delete frontend_server;
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
        currentProcessor = pipeline_data->pipeline_processors->find(pipeline_data->first_request_pipeline_processor_index)->second;
    } else {
        Logger::info << "Started response pipeline" << endl;
        if (pipeline_data->first_response_pipeline_processor_index == -1) {
            currentProcessor = nullptr;
        } else {
            currentProcessor = pipeline_data->pipeline_processors->find(pipeline_data->first_response_pipeline_processor_index)->second;
        }
        is_response = true;
    }

    while (currentProcessor != nullptr) {
        currentProcessor = currentProcessor->processAndGetNextProcessor(data);
    }

    if (is_response) {
        Logger::info << "Starting return to frontend server" << endl;
        frontend_server->processResponse(data->getClientSocket(), client_connection_module->handleWrite(ClientSocketCommunicator::getConnectionModuleDataCarrier(data->getClientSocket()), data));
    }
}

void ProcessingPipeline::setInitializationData(string name, InitializationData* data) {
    // TODO: move something here?
}

ProcessingPipelineData * ProcessingPipeline::getProcessingPipelineData() {
    return pipeline_data;
}

void PipelineProcessor::setProcessingPipelineData(ProcessingPipelineData* pipeline_data) {
    processing_pipeline_data = pipeline_data;
}