#pragma once
#include <map>
#include <vector>

class PipelineProcessor;
namespace luabridge {
    class LuaRef;
}
class InitializationData;
struct lua_State;
struct ProcessingPipelineData;
class ClientConnectionModule;

using namespace std;

class LuaConfigurationProcessor {
    public:
        LuaConfigurationProcessor(string lua_file_name);
        void loadLibrariesFrom(string path);
        void initializeProcessorList(EPollManager* v_epoll_manager);
        void initializePipelinePortList();
        map<int, PipelineProcessor*>* getAllProcessors();
        vector<string>* getAllPorts();
        int getFirstProcessorIDForPort(string port);
        ClientConnectionModule* getConnectionForPort(string port);
        ~LuaConfigurationProcessor();
    private:
        void createProcessors(map<string, luabridge::LuaRef>* requetsed_processor_map, EPollManager* v_epoll_manager);
        void passProcessorParameters(map<string, luabridge::LuaRef>* requetsed_processor_map);
        InitializationData* reinterpretData(luabridge::LuaRef data);
        lua_State* L;
        map<int, PipelineProcessor*>* pipeline_processors = nullptr;
        vector<string>* ports;
        map<string, pair<int, ClientConnectionModule*>>* port_to_processor_mapping = nullptr;
};
