#include "libyslow.h"

PipelineProcessor* RequestPipelineProcessor::processAndGetNextProcessor(ProcessingPipelinePacket* data) {
    return processRequest(data);
}

PipelineProcessor* ResponsePipelineProcessor::processAndGetNextProcessor(ProcessingPipelinePacket* data) {
    return processResponse(data);
}

void InitializationData::setString(string v_data) {
    data_type = TYPE_STRING;
    data = &v_data;
}

void InitializationData::setInt(int v_data) {
    data_type = TYPE_INT;
    data = &v_data;
}

void InitializationData::setMap(map<string, InitializationData*>* v_data) {
    data_type = TYPE_MAP;
    data = &v_data;
}

void InitializationData::setProcessor(PipelineProcessor* v_data) {
    data_type = TYPE_PROCESSOR;
    data = v_data;
}

bool InitializationData::isString() {
    return data_type == TYPE_STRING;
}

bool InitializationData::isInt() {
    return data_type == TYPE_INT;
}

bool InitializationData::isMap() {
    return data_type == TYPE_MAP;
}

bool InitializationData::isProcessor() {
    return data_type == TYPE_PROCESSOR;
}

string InitializationData::getString() {
    if (!isString()) {
        return nullptr;
    }
    return *static_cast<string*>(data);
}

int InitializationData::getInt() {
    if (!isInt()) {
        return 0;
    }
    return *static_cast<int*>(data);
}

map<string, InitializationData*>* InitializationData::getMap() {
    if (!isMap()) {
        return nullptr;
    }
    return static_cast<map<string, InitializationData*>*>(data);
}

PipelineProcessor* InitializationData::getProcessor() {
    if (!isProcessor()) {
        return nullptr;
    }
    return static_cast<PipelineProcessor*>(data);
}

void ConnectionModuleDataCarrier::setConnectionModuleData(void * v_data) {
    connection_module_data = v_data;
}

void* ConnectionModuleDataCarrier::getConnectionModuleData() {
    return connection_module_data;
}

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

void ProcessingPipelinePacket::setFirstResponsePipelineProcessor(ResponsePipelineProcessor* v_processor) {
    first_response_processor = v_processor;
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

PipelineProcessor* ProcessingPipelinePacket::getFirstResponsePipelineProcessor() {
    return first_response_processor;
}

ClientConnectionModule::ClientConnectionModule() {
}

void ClientConnectionModule::setPipelineData(ProcessingPipelineData* v_pipeline_data) {
    pipeline_data = v_pipeline_data;
}

ProcessingPipelineData* ClientConnectionModule::getPipelineData() {
    return pipeline_data;
}