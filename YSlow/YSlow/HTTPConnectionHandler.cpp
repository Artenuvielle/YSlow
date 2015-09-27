#include "../libyslow/Logger.h"
#include "../httpxx/ResponseBuilder.hpp"
#include "HTTPConnectionHandler.h"

using namespace std;

struct HTTPConnectionModuleData {
    http::BufferedRequest* request;
};

HTTPClientConnectionModule::HTTPClientConnectionModule() {
}

HTTPClientConnectionModule::~HTTPClientConnectionModule() {
}

void HTTPClientConnectionModule::handleRead(ConnectionModuleDataCarrier* socket, char* read_buffer, int bytes_read) {
    Logger::info << "Got: " << bytes_read << " bytes:" << endl << string(read_buffer, bytes_read) << endl;
    HTTPConnectionModuleData* current_request_data = ((HTTPConnectionModuleData*)socket->getConnectionModuleData());
    if (current_request_data == nullptr) {
        current_request_data = new HTTPConnectionModuleData{ nullptr };
        socket->setConnectionModuleData(current_request_data);
    }
    if (current_request_data->request == nullptr) {
        current_request_data->request = new http::BufferedRequest();
    }

    int parsed_bytes = 0;
    while (parsed_bytes < bytes_read) {
        int parsed_bytes_in_cycle = current_request_data->request->feed(read_buffer + parsed_bytes, bytes_read - parsed_bytes);
        parsed_bytes += parsed_bytes_in_cycle;
    }
}

ProcessingPipelinePacket* HTTPClientConnectionModule::getCompletePacket(ConnectionModuleDataCarrier * socket) {
    HTTPConnectionModuleData* current_request_data = ((HTTPConnectionModuleData*)socket->getConnectionModuleData());
    if (current_request_data == nullptr) {
        current_request_data = new HTTPConnectionModuleData{ nullptr };
        socket->setConnectionModuleData(current_request_data);
    }
    if (current_request_data->request == nullptr) {
        current_request_data->request = new http::BufferedRequest();
    }
    if (current_request_data->request->complete()) {
        ProcessingPipelinePacket* packet = new ProcessingPipelinePacket(PIPELINE_REQUEST);
        packet->setPacketRequestData(current_request_data->request);
        current_request_data->request = nullptr;
        return packet;
    }
    return nullptr;
}

string HTTPClientConnectionModule::handleWrite(ConnectionModuleDataCarrier* socket, ProcessingPipelinePacket* packet) {
    if (packet->getPacketType() == PIPELINE_RESPONSE) {
        http::BufferedResponse* response = packet->getPacketResponseData();
        http::ResponseBuilder* responseBuilder = new http::ResponseBuilder(*response);
        string ret = responseBuilder->to_string() + response->body();
        delete responseBuilder;
        return ret;
    }
    return string();
}