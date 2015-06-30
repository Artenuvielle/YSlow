#include <stdlib.h>
#include <signal.h>
#include <sys/epoll.h>
#include <lua.hpp>

#include "EPoll.h"
#include "ProcessingPipeline.h"
#include "Logger.h"

using namespace std;

char* get_config_opt(lua_State* L, char* name) {
    lua_getglobal(L, name);
    if (!lua_isstring(L, -1)) {
        Logger::error << name << " must be a string" << endl;
        exit(1);
    }
    return (char*)lua_tostring(L, -1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        Logger::error << "Usage: " << argv[0] << " <config_file>" << endl;
        exit(1);
    }
    cout << "---------------------------------------------" << endl;
    cout << "------------ YSlow Reverse Proxy ------------" << endl;
    cout << "---------------------------------------------" << endl << endl;

    lua_State *L = lua_open();

    if (luaL_dofile(L, argv[1]) != 0) {
        Logger::error << "Error parsing config file: " << lua_tostring(L, -1) << endl;
        exit(1);
    }
    char* server_port_str = get_config_opt(L, "listenPort");
    char* backend_address = get_config_opt(L, "backendAddress");
    char* backend_port_str = get_config_opt(L, "backendPort");

    signal(SIGPIPE, SIG_IGN);

    ProcessingPipelineData* pipeline_data = new ProcessingPipelineData;
    pipeline_data->epoll_manager = new EPollManager();
    pipeline_data->frontend_server_port_string = server_port_str;
    pipeline_data->backend_server_host_string = backend_address;
    pipeline_data->backend_server_port_string = backend_port_str;

    ProcessingPipeline* main_pipeline = new ProcessingPipeline(pipeline_data);
    main_pipeline->setupPipeline();
    delete main_pipeline;

    //create_server_socket_handler(server_port_str, backend_address, backend_port_str);

    return 0;
}