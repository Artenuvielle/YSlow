#pragma once
#include "../libyslow/libyslow.h"

class ConditionModule : public RequestPipelineProcessor {
    public:
        PipelineProcessor* processRequest(ProcessingPipelinePacket* data) override;
        void setInitializationData(string name, InitializationData* data) override;
    private:
        bool testHeaderData(string header);
        PipelineProcessor* onTrueProcessor = nullptr;
        PipelineProcessor* onFalseProcessor = nullptr;
        string headerField = "";
        string matcher = "";
        bool useRegex = false;
};

extern "C" {
    inline PipelineProcessor* createConditionModule() {
        return new ConditionModule;
    }
    class condition_module_proxy {
        public:
            condition_module_proxy() {
                module_library["condition_module"] = createConditionModule;
            }
    };
    condition_module_proxy p;
}