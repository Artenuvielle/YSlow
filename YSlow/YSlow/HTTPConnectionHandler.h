#pragma once
#include "../libyslow/libyslow.h"

struct ProcessingPipelineData;

class HTTPClientConnectionModule : public ClientConnectionModule {
    public:
        HTTPClientConnectionModule();
        ~HTTPClientConnectionModule();
        bool handleRead(ProcessingPipelinePacket* packet, char* read_buffer, int bytes_read) override;
        string handleWrite(ProcessingPipelinePacket* packet) override;
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