#include <stdlib.h>
#include <signal.h>
#include <sstream>

#include "../libyslow/Logger.h"
#include "EPoll.h"
#include "ConfigurationProcessor.h"
#include "ProcessingPipeline.h"

#include "Network.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        Logger::error << "Usage: " << argv[0] << " <config_file> [<library_directory>]" << endl;
        exit(1);
    }
    cout << "---------------------------------------------" << endl;
    cout << "------------ YSlow Reverse Proxy ------------" << endl;
    cout << "---------------------------------------------" << endl << endl;

    string library_path;

    if (argc == 2) {
        string executable_path = (argv[0]);
        stringstream path_stream;
        path_stream << executable_path.substr(0, executable_path.rfind('/') + 1) << "ModuleLibraries/";
        library_path = path_stream.str();
    } else {
        library_path = string(argv[2]);
    }

    EPollManager* epoll_manager = new EPollManager();

    LuaConfigurationProcessor* configuration_processor = new LuaConfigurationProcessor(argv[1]);
    configuration_processor->loadLibrariesFrom(library_path);
    configuration_processor->initializeProcessorList(epoll_manager);
    configuration_processor->initializePipelinePortList();
    PipelineProcessor* pipeline_modules;

    signal(SIGPIPE, SIG_IGN);

    BackendServer* test = new BackendServer(epoll_manager, "localhost", "3000");
    test->request("GET http://localhost:3000/ HTTP/1.0");

    vector<string>* ports = configuration_processor->getAllPorts();
    for (int i = 0; i < ports->size(); i++) {
        ProcessingPipelineData* pipeline_data = new ProcessingPipelineData;
        pipeline_data->epoll_manager = epoll_manager;
        pipeline_data->frontend_server_port_string = ports->at(i);
        pipeline_data->pipeline_processors = configuration_processor->getAllProcessors();
        pipeline_data->first_request_pipeline_processor_index = configuration_processor->getFirstRequestProcessorIDForPort(ports->at(i));
        pipeline_data->first_response_pipeline_processor_index = configuration_processor->getFirstResponseProcessorIDForPort(ports->at(i));
        pipeline_data->connection_module = configuration_processor->getConnectionForPort(ports->at(i));

        ProcessingPipeline* main_pipeline = new ProcessingPipeline(pipeline_data);
        main_pipeline->setupPipeline();
        delete main_pipeline;
    }

    return 0;
}