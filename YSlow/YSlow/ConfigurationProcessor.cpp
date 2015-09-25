#include <stdlib.h>
#include <sstream>
#include <dlfcn.h>
#include "../LuaBridge/LuaBridge.h"
#include "../libyslow/libyslow.h"
#include "lua.h"

#include "Logger.h"
#include "Network.h"
#include "ConfigurationProcessor.h"

using namespace luabridge;

map<string, module_maker_t*, less<string> > module_library;
map<string, connection_maker_t*, less<string> > connection_library;

class LuaIterator {
    public:
        LuaIterator(LuaRef ref, lua_State* v_L) : L(v_L) {
            ref.push(L);
            push(L, Nil());
            is_first = true;
        }
        bool next() {
            if (!is_first) {
                lua_pop(L, 1);
            } else {
                is_first = false;
            }
            return (lua_next(L, -2) != 0);
        }
        LuaRef getKey() {
            return LuaRef::fromStack(L, -2);
        }
        LuaRef getValue() {
            return LuaRef::fromStack(L, -1);
        }
    private:
        lua_State* L;
        bool is_first;
};

map<string, LuaRef>* getLuaTable(LuaRef lua_refferer, lua_State* L) {
    map<string, LuaRef>* mapped_table = new map<string, LuaRef>();
    LuaIterator* iterator = new LuaIterator(lua_refferer, L);
    while (iterator->next()) {
        mapped_table->insert(pair<string, LuaRef>(iterator->getKey().cast<string>(), iterator->getValue()));
    }
    delete iterator;
    return mapped_table;
}

LuaConfigurationProcessor::LuaConfigurationProcessor(string lua_file_name) {
    L = lua_open();

    ports = new vector<string>();

    if (luaL_dofile(L, lua_file_name.c_str()) != 0) {
        Logger::error << "Error parsing lua config file: " << lua_tostring(L, -1) << endl;
        exit(1);
    } else {
        Logger::info << "Read config file " << lua_file_name << endl;
    }
}

void LuaConfigurationProcessor::loadLibrariesFrom(string path) {
    Logger::info << "Loading libraries from path \"" << path << "\"" << endl;
    LuaRef requested_module_libraries = getGlobal(L, "RequiredLibraries");
    map<string, LuaRef>* requested_module_libraries_map = getLuaTable(requested_module_libraries, L);
    for (map<string, LuaRef>::iterator it = requested_module_libraries_map->begin(); it != requested_module_libraries_map->end(); ++it) {
        stringstream library_file_name;
        library_file_name << path << it->second.cast<string>() << ".so";
        void *hndl = dlopen(library_file_name.str().c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (hndl == nullptr) {
            Logger::error << "Could not load library " << it->second.cast<string>() << ": " << dlerror() << endl;
            exit(-1);
        } else {
            Logger::info << "Loaded library '" << it->second.cast<string>() << ".so'" << endl;
        }
    }
}

void LuaConfigurationProcessor::initializeProcessorList(EPollManager* v_epoll_manager) {
    if (pipeline_processors != nullptr) {
        delete pipeline_processors;
    }
    pipeline_processors = new map<int, PipelineProcessor*>();
    LuaRef requested_pipeline_modules = getGlobal(L, "PipelineModules");
    map<string, LuaRef>* requested_pipeline_modules_map = getLuaTable(requested_pipeline_modules, L);
    Logger::info << "Creating " << requested_pipeline_modules_map->size() << " instances of modules" << endl;
    createProcessors(requested_pipeline_modules_map, v_epoll_manager);
    passProcessorParameters(requested_pipeline_modules_map);
    delete requested_pipeline_modules_map;
}

void LuaConfigurationProcessor::initializePipelinePortList() {
    if (port_to_processor_mapping != nullptr) {
        delete port_to_processor_mapping;
    }
    port_to_processor_mapping = new std::map<string, pair<int, ClientConnectionModule*>>();

    LuaRef pipeline_ports = getGlobal(L, "PipelinePorts");
    map<string, LuaRef>* pipeline_ports_map = getLuaTable(pipeline_ports, L);
    for (map<string, LuaRef>::iterator it = pipeline_ports_map->begin(); it != pipeline_ports_map->end(); ++it) {
        string requested_connection_handler_name = it->second["connection"].cast<string>();
        if (connection_library.find(requested_connection_handler_name) == connection_library.end()) {
            Logger::error << "Tried to instatiate unloaded connection handler " << requested_connection_handler_name << endl;
            exit(-1);
        }
        ClientConnectionModule* connection_module = connection_library[requested_connection_handler_name]();
        string port = it->second["port"].cast<string>();
        ports->push_back(port);
        port_to_processor_mapping->insert(pair<string, pair<int, ClientConnectionModule*>>(port, pair<int, ClientConnectionModule*>(it->second["module"].cast<int>(), connection_module)));
    }
}

map<int, PipelineProcessor*>* LuaConfigurationProcessor::getAllProcessors() {
    return pipeline_processors;
}

vector<string>* LuaConfigurationProcessor::getAllPorts() {
    return ports;
}

int LuaConfigurationProcessor::getFirstProcessorIDForPort(string port) {
    return port_to_processor_mapping->at(port).first;
}

ClientConnectionModule* LuaConfigurationProcessor::getConnectionForPort(string port) {
    return port_to_processor_mapping->at(port).second;
}

LuaConfigurationProcessor::~LuaConfigurationProcessor() {
    delete[] pipeline_processors;
    delete ports;
    delete port_to_processor_mapping;
}

void LuaConfigurationProcessor::createProcessors(map<string, luabridge::LuaRef>* requetsed_processor_map, EPollManager* v_epoll_manager) {
    for (map<string, LuaRef>::iterator it = requetsed_processor_map->begin(); it != requetsed_processor_map->end(); ++it) {
        map<string, LuaRef>* module_data = getLuaTable(it->second, L);
        Logger::debug << "Current type: " << module_data->at("type").cast<string>() << endl;
        string requested_module_name = module_data->at("type").cast<string>();
        if (requested_module_name == "backend_server") {
            string host = module_data->at("host").cast<string>();
            string port = module_data->at("port").cast<string>();
            pipeline_processors->insert(pair<int, PipelineProcessor*>(atoi(it->first.c_str()), new BackendServer(v_epoll_manager, host.c_str(), port.c_str())));
            Logger::info << "Created backend server at index " << atoi(it->first.c_str()) << endl;
        } else {
            if (module_library.find(requested_module_name) == module_library.end()) {
                Logger::error << "Tried to instatiate unloaded module " << requested_module_name << endl;
                exit(-1);
            }
            int module_index = atoi(it->first.c_str());
            PipelineProcessor* proc = module_library[requested_module_name]();
            pipeline_processors->insert(pair<int, PipelineProcessor*>(module_index, proc));
            for (map<string, LuaRef>::iterator data_iterator = module_data->begin(); data_iterator != module_data->end(); ++data_iterator) {
                proc->setInitializationData(data_iterator->first, reinterpretData(data_iterator->second));
            }
        }
    }
}

void LuaConfigurationProcessor::passProcessorParameters(map<string, luabridge::LuaRef>* requetsed_processor_map) {
    for (map<string, LuaRef>::iterator it = requetsed_processor_map->begin(); it != requetsed_processor_map->end(); ++it) {
        map<string, LuaRef>* module_data = getLuaTable(it->second, L);
        int module_index = atoi(it->first.c_str());
        for (map<string, LuaRef>::iterator data_iterator = module_data->begin(); data_iterator != module_data->end(); ++data_iterator) {
            pipeline_processors->at(module_index)->setInitializationData(data_iterator->first, reinterpretData(data_iterator->second));
        }
    }
}

InitializationData* LuaConfigurationProcessor::reinterpretData(LuaRef data) {
    InitializationData* common_data = new InitializationData();
    if (data.isString()) {
        common_data->setString(data.cast<string>());
    } else if (data.isNumber()) {
        common_data->setInt(data.cast<int>());
    } else if (data.isTable()) {
        map<string, InitializationData*>* recursive_reinterpreted_data = new map<string, InitializationData*>();
        map<string, LuaRef>* table_map = getLuaTable(data, L);
        if (table_map->size() == 1 && table_map->count("processor") == 1) {
            common_data->setProcessor(pipeline_processors->at(table_map->at("processor").cast<int>()));
        } else {
            for (map<string, LuaRef>::iterator it = table_map->begin(); it != table_map->end(); ++it) {
                recursive_reinterpreted_data->insert(pair<string, InitializationData*>(it->first, reinterpretData(it->second)));
            }
            common_data->setMap(recursive_reinterpreted_data);
        }
    }
    return common_data;
}