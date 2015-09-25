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

ClientConnectionModule::ClientConnectionModule() {
}

void ClientConnectionModule::setPipelineData(ProcessingPipelineData* v_pipeline_data) {
    pipeline_data = v_pipeline_data;
}

ProcessingPipelineData * ClientConnectionModule::getPipelineData() {
    return pipeline_data;
}