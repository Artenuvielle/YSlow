#include "Frame.h"

namespace wsxx {
    char* WebSocketFrame::getData() {
        return data;
    }

    void WebSocketFrame::setData(char* v_data) {
        data = v_data;
    }

    long WebSocketFrame::getDataLength() {
        return data_length;
    }

    void WebSocketFrame::setDataLength(int v_data_length) {
        data_length = v_data_length;
    }

    bool WebSocketFrame::isFin() {
        return fin;
    }

    void WebSocketFrame::setFin(bool v_fin) {
        fin = v_fin;
    }

    bool WebSocketFrame::isRSV1() {
        return rsv_1;
    }

    void WebSocketFrame::setRSV1(bool v_rsv_1) {
        rsv_1 = v_rsv_1;
    }

    bool WebSocketFrame::isRSV2() {
        return rsv_2;
    }

    void WebSocketFrame::setRSV2(bool v_rsv_2) {
        rsv_2 = v_rsv_2;
    }

    bool WebSocketFrame::isRSV3() {
        return rsv_3;
    }

    void WebSocketFrame::setRSV3(bool v_rsv_3) {
        rsv_3 = v_rsv_3;
    }

    int WebSocketFrame::getOpcode() {
        return opcode;
    }

    void WebSocketFrame::setOpcode(int v_opcode) {
        opcode = v_opcode;
    }
}