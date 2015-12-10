#include "RoundRobinModule.h"

PipelineProcessor* RoundRobinModule::processRequest(ProcessingPipelinePacket* data) {
    if (nexts != nullptr) {
        if (next_iterator == nexts->end()) {
            next_iterator = nexts->begin();
        }
        PipelineProcessor* proc = next_iterator->second;
        next_iterator++;
        return proc;
    } else {
        return nullptr;
    }
}

void RoundRobinModule::setInitializationData(string name, InitializationData* data) {
    if (name == "next" && data->isMap()) {
        nexts = new map<string, PipelineProcessor*>();
        map<string, InitializationData*>* m = data->getMap();

        for (map<string, InitializationData*>::iterator iterator = m->begin(); iterator != m->end(); iterator++) {
            if (iterator->second->isProcessor()) {
                nexts->insert(pair<string, PipelineProcessor*>(iterator->first, iterator->second->getProcessor()));
            }
        }
        next_iterator = nexts->begin();
    }
}