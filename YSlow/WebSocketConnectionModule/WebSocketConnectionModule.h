#pragma once
#include "../libyslow/libyslow.h"

class WebScoketConnectionModule : public ClientConnectionModule {
    public:
        WebScoketConnectionModule();
        ~WebScoketConnectionModule();
        void handleRead(ConnectionModuleDataCarrier* socket, char* read_buffer, int bytes_read) override;
        ProcessingPipelinePacket* getCompletePacket(ConnectionModuleDataCarrier* socket) override;
        string handleWrite(ConnectionModuleDataCarrier* socket, ProcessingPipelinePacket* packet) override;
};

extern "C" {
    inline ClientConnectionModule* createWebScoketConnectionModule() {
        return new WebScoketConnectionModule;
    }
    class websocket_connection_proxy {
        public:
            websocket_connection_proxy() {
                connection_library["ws"] = createWebScoketConnectionModule;
            }
    };
    websocket_connection_proxy p;
}