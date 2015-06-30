#include <stdint.h>
#include <stdlib.h>

#include "Logger.h"
#include "ProcessingPipeline.h"
#include "ConnectionHandler.h"

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

ProcessingPipelinePacket* HTTPClientConnectionModule::handleRead(char* read_buffer, int bytes_read) {
    Logger::info << "Got: " << bytes_read << " bytes" << endl;
    ProcessingPipelinePacket* packet_data = new ProcessingPipelinePacket(PIPELINE_REQUEST);
    packet_data->setPacketData(read_buffer, bytes_read / sizeof(char));
    return packet_data;
}

string HTTPClientConnectionModule::handleWrite(ProcessingPipelinePacket * packet) {
    return string();
}