#include "Logger.h"
#include "ProcessingPipeline.h"
#include "ConnectionHandler.h"
#include "../httpxx/Response.hpp"
#include "../httpxx/ResponseBuilder.hpp"

using namespace std;

ClientConnectionModule::ClientConnectionModule(ProcessingPipelineData* v_pipeline_data) : pipeline_data(v_pipeline_data) {
}

ProcessingPipelineData * ClientConnectionModule::getPipelineData() {
    return pipeline_data;
}

HTTPClientConnectionModule::HTTPClientConnectionModule(ProcessingPipelineData* pipeline_data) : ClientConnectionModule(pipeline_data) {
}

HTTPClientConnectionModule::~HTTPClientConnectionModule() {
}

bool HTTPClientConnectionModule::handleRead(ProcessingPipelinePacket* packet, char* read_buffer, int bytes_read) {
    Logger::info << "Got: " << bytes_read << " bytes" << endl;
    http::BufferedRequest* current_request = packet->getPacketRequestData();
    if (current_request == nullptr) {
        current_request = new http::BufferedRequest();
        packet->setPacketRequestData(current_request);
    }
    current_request->feed(read_buffer, bytes_read);
    return current_request->complete();
}

string HTTPClientConnectionModule::handleWrite(ProcessingPipelinePacket* packet) {
    if (packet->getPacketType() == PIPELINE_RESPONSE) {
        http::BufferedResponse* response = packet->getPacketResponseData();
        http::ResponseBuilder* responseBuilder = new http::ResponseBuilder(*response);
        string ret = responseBuilder->to_string() + response->body();
        delete responseBuilder;
        return ret;
    }
    return string();
}