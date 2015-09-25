#include "Logger.h"
#include "ProcessingPipeline.h"
#include "HTTPConnectionHandler.h"
#include "../httpxx/ResponseBuilder.hpp"

using namespace std;

HTTPClientConnectionModule::HTTPClientConnectionModule() {
}

HTTPClientConnectionModule::~HTTPClientConnectionModule() {
}

bool HTTPClientConnectionModule::handleRead(ProcessingPipelinePacket* packet, char* read_buffer, int bytes_read) {
    Logger::info << "Got: " << bytes_read << " bytes:" << endl << string(read_buffer, bytes_read) << endl;
    http::BufferedRequest* current_request = packet->getPacketRequestData();
    if (current_request == nullptr) {
        current_request = new http::BufferedRequest();
        packet->setPacketRequestData(current_request);
    }

    int parsed_bytes = 0;
    while (parsed_bytes < bytes_read) {
        int parsed_bytes_in_cycle = current_request->feed(read_buffer + parsed_bytes, bytes_read - parsed_bytes);
        parsed_bytes += parsed_bytes_in_cycle;
    }

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