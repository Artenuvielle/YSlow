#pragma once

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
            WebSocketFrame();
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
            long data_length;
            bool fin;
            bool rsv_1;
            bool rsv_2;
            bool rsv_3;
            int opcode;
    };
}