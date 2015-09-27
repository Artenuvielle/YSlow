#pragma once
#include <bitset>

namespace wsxx {
    enum OPCODE {
        TYPE_CONTINUATION_FRAME = 0x0,
        TYPE_TEXT_FRAME = 0x1,
        TAPE_BINARY_FRAME = 0x2,
        CTRL_CLOSE_FRAME = 0x8,
        CTRL_PING_FRAME = 0x9,
        CTRL_PONG_FRAME = 0xA
    };

    class WebSocketFrame {
        public:
            char* getData();
            void setData(char* v_data);
            long getDataLength();
            void setDataLength(int v_data_length);
            bool isFin();
            void setFin(bool v_fin);
            bool isRSV1();
            void setRSV1(bool v_rsv_1);
            bool isRSV2();
            void setRSV2(bool v_rsv_2);
            bool isRSV3();
            void setRSV3(bool v_rsv_3);
            int getOpcode();
            void setOpcode(int v_opcode);
        private:
            char* data;
            long data_length = 0;
            bool fin = false;
            bool rsv_1 = false;
            bool rsv_2 = false;
            bool rsv_3 = false;
            int opcode = 0;
    };

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

    class WebSocketParser {
        public:
            WebSocketParser();
            void feed(const char* data, size_t length);
            WebSocketFrame* getComplete();
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
            bool masking_key[32];
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