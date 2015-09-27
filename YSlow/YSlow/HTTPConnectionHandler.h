#pragma once
#include "../libyslow/libyslow.h"

struct ProcessingPipelineData;

class HTTPClientConnectionModule : public ClientConnectionModule {
    public:
        HTTPClientConnectionModule();
        ~HTTPClientConnectionModule();
        void handleRead(ConnectionModuleDataCarrier* socket, char* read_buffer, int bytes_read) override;
        ProcessingPipelinePacket* getCompletePacket(ConnectionModuleDataCarrier* socket) override;
        string handleWrite(ConnectionModuleDataCarrier* socket, ProcessingPipelinePacket* packet) override;
};

extern "C" {
    inline ClientConnectionModule* createHTTPConnectionModule() {
        return new HTTPClientConnectionModule();
    }
    class http_connection_proxy {
        public:
            http_connection_proxy() {
                connection_library["http"] = createHTTPConnectionModule;
            }
    };
    http_connection_proxy p;
}