#include <regex>

#include "RoundRobinModule.h"

PipelineProcessor* RoundRobinModule::processRequest(ProcessingPipelinePacket* data) {
    if (data->getPacketRequestData()->has_header(headerField) && this->testHeaderData(data->getPacketRequestData()->header(headerField))) {
        return onTrueProcessor;
    } else {
        return onFalseProcessor;
    }
}

void RoundRobinModule::setInitializationData(string name, InitializationData* data) {
    if (name == "trueCase" && data->isProcessor()) {
        onTrueProcessor = data->getProcessor();
    } else if (name == "falseCase" && data->isProcessor()) {
        onFalseProcessor = data->getProcessor();
    } else if (name == "header" && data->isString()) {
        headerField = data->getString();
    } else if (name == "matcher" && data->isString()) {
        matcher = data->getString();
    } else if (name == "isRegex" && data->isInt()) {
        useRegex = (data->getInt() != 0);
    }
}

bool RoundRobinModule::testHeaderData(string header) {
    return useRegex ? (std::regex_match(header, std::regex(matcher))) : (header == matcher);
}