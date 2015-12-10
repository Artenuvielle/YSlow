#pragma once
#include "../libyslow/libyslow.h"

class RoundRobinModule : public RequestPipelineProcessor {
    public:
        PipelineProcessor* processRequest(ProcessingPipelinePacket* data) override;
        void setInitializationData(string name, InitializationData* data) override;
    private:
        bool testHeaderData(string header);
        map<string, PipelineProcessor*>* nexts = nullptr;
        map<string, PipelineProcessor*>::iterator next_iterator;
};

extern "C" {
    inline PipelineProcessor* createRoundRobinModule() {
        return new RoundRobinModule;
    }
    class round_robin_module_proxy {
        public:
            round_robin_module_proxy() {
                module_library["round_robin_module"] = createRoundRobinModule;
            }
    };
    round_robin_module_proxy p;
}