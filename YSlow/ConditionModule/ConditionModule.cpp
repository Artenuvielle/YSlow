#include "ConditionModule.h"

PipelineProcessor* ConditionModule::processRequest(ProcessingPipelinePacket* data) {
    return nextProcessor;
}

void ConditionModule::setInitializationData(string name, InitializationData* data) {
    if (name == "next" && data->isProcessor()) {
        nextProcessor = data->getProcessor();
    }
}