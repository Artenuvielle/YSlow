#pragma once
#include <bitset>

namespace wsxx {
    class WebSocketFrame;

    struct WebsocketFrameListElement {
        WebSocketFrame* frame;
        WebsocketFrameListElement* next;
    };

    enum ParserState {
        FIN,
        RSV1,
        RSV2,
        RSV3,
        OPCODE,
        MASK,
        LEN,
        LEN_EXT,
        LEN_EXT_CON,
        MASK_KEY,
        DATA
    };

    class WebsocketParser {
        public:
            WebsocketParser();
            void feed(const char* data, size_t length);
            WebSocketFrame* complete();
        private:
            void interpretBit(bool bit);
            WebSocketFrame* current = nullptr;
            ParserState state = FIN;
            int bit_read = 0;
            long length;
            int opcode;
            bool is_masked;
            char* data = nullptr;
            long data_offset = 0;
            char mask_offset = 0;
            std::bitset<32> masking_key;
            WebsocketFrameListElement* completed_list = nullptr;
    };

    class WebSocketFrameBuilder {
        public:
            WebSocketFrameBuilder(WebSocketFrame* v_frame);
            std::string toString(bool useMask);
        private:
            WebSocketFrame* frame;
    };
}