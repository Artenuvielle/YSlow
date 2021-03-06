//#include <stdlib.h>
#include <limits.h>
#include <time.h>

#include "WebsocketParser.h"

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

    WebSocketParser::WebSocketParser() {
    }

    void WebSocketParser::feed(const char* data, size_t length) {
        for (int i = 0; i < length; i++) {
            char cur = data[i];
            for (int a = 0; a < CHAR_BIT; a++) {
                bool bit = (cur >> (CHAR_BIT - 1 - a)) & 1;
                interpretBit(bit);
            }
        }
    }

    WebSocketFrame* WebSocketParser::getComplete() {
        if (completed_list != nullptr) {
            WebSocketFrame* ret = completed_list->frame;
            WebsocketFrameListElement* el = completed_list;
            completed_list = completed_list->next;
            delete el;
            return ret;
        }
        return nullptr;
    }

    void WebSocketParser::interpretBit(bool bit) {
        if (state == FIN) {
            if (current == nullptr) {
                current = new WebSocketFrame();
                current->setFin(bit);
                state = RSV1;
            } else {
                // EXCEPTION
            }
        } else if (state == RSV1) {
            current->setRSV1(bit);
            state = RSV2;
        } else if (state == RSV2) {
            current->setRSV2(bit);
            state = RSV3;
        } else if (state == RSV3) {
            current->setRSV3(bit);
            opcode = 0;
            state = OPCODE;
        } else if (state == OPCODE) {
            opcode = (opcode << 1) | bit;
            bit_read++;
            if (bit_read >= 4) {
                current->setOpcode(opcode);
                bit_read = 0;
                opcode = 0;
                state = MASK;
            }
        } else if (state == MASK) {
            is_masked = bit;
            length = 0;
            state = LEN;
        } else if (state == LEN) {
            length = (length << 1) | bit;
            bit_read++;
            if (bit_read >= 7) {
                if (length == 126) {
                    state = LEN_EXT;
                } else if (length == 127) {
                    state = LEN_EXT_CON;
                } else {
                    current->setDataLength(length);
                    if (is_masked) {
                        state = MASK_KEY;
                    } else {
                        state = DATA;
                    }
                }
                bit_read = 0;
                length = 0;
            }
        } else if (state == LEN_EXT) {
            length = (length << 1) | bit;
            bit_read++;
            if (bit_read >= 16) {
                current->setDataLength(length);
                if (is_masked) {
                    state = MASK_KEY;
                } else {
                    state = DATA;
                }
                bit_read = 0;
                length = 0;
            }
        } else if (state == LEN_EXT_CON) {
            // UNSUPPORTED
            current->setDataLength(0xAAAAAAAA);
            if (is_masked) {
                state = MASK_KEY;
            } else {
                state = DATA;
            }
        } else if (state == MASK_KEY) {
            masking_key[bit_read] = bit;
            bit_read++;
            if (bit_read >= 32) {
                bit_read = 0;
                length = 0;
                state = DATA;
            }
        } else if (state == DATA) {
            if (data == nullptr) {
                data = new char[current->getDataLength()];
            }
            if (is_masked) {
                length = (length << 1) | (bit ^ masking_key[mask_offset]);
            } else {
                length = (length << 1) | bit;
            }
            bit_read++;
            mask_offset = (mask_offset + 1) % 32;
            if (bit_read >= 8) {
                data[data_offset] = length;
                data_offset++;
                bit_read = 0;
                length = 0;
                if (data_offset >= current->getDataLength()) {
                    current->setData(data);
                    mask_offset = 0;
                    data_offset = 0;
                    data = nullptr;
                    if (completed_list == nullptr) {
                        completed_list = new WebsocketFrameListElement{ current, nullptr };
                    } else {
                        WebsocketFrameListElement* last = completed_list;
                        while (last->next != nullptr) {
                            last = last->next;
                        }
                        last->next = new WebsocketFrameListElement{ current, nullptr };
                    }
                    current = nullptr;
                    state = FIN;
                }
            }
        }
    }
    WebSocketFrameBuilder::WebSocketFrameBuilder(WebSocketFrame* v_frame) : frame(v_frame) {
    }
    std::string WebSocketFrameBuilder::toString(bool useMask) {
        int header_size = 2;
        if (frame->getDataLength() >= 126) {
            header_size += 2;
        }
        if (useMask) {
            header_size += 4;
        }
        char* frame_sequence = new char[header_size + frame->getDataLength()];
        long frame_offset = 0;
        frame_sequence[frame_offset] = frame->isFin();
        frame_sequence[frame_offset] = (frame_sequence[frame_offset] << 1) | frame->isRSV1();
        frame_sequence[frame_offset] = (frame_sequence[frame_offset] << 1) | frame->isRSV2();
        frame_sequence[frame_offset] = (frame_sequence[frame_offset] << 1) | frame->isRSV3();
        frame_sequence[frame_offset] = (frame_sequence[frame_offset] << 4) | frame->getOpcode();
        frame_offset++;
        frame_sequence[frame_offset] = useMask;
        if (frame->getDataLength() < 126) {
            frame_sequence[frame_offset] = (frame_sequence[frame_offset] << 7) | frame->getDataLength();
            frame_offset++;
        } else {
            // lengths over 2^32 Bit unsupported
            frame_sequence[frame_offset] = (frame_sequence[frame_offset] << 7) | 0x7E;
            frame_offset++;
            frame_sequence[frame_offset] = (frame->getDataLength() >> 8);
            frame_offset++;
            frame_sequence[frame_offset] = (frame->getDataLength());
            frame_offset++;
        }
        if (useMask) {
            srand(time(NULL));
            char mask[4];
            mask[0] = rand();
            mask[1] = rand();
            mask[2] = rand();
            mask[3] = rand();
            frame_sequence[frame_offset] = mask[0];
            frame_offset++;
            frame_sequence[frame_offset] = mask[1];
            frame_offset++;
            frame_sequence[frame_offset] = mask[2];
            frame_offset++;
            frame_sequence[frame_offset] = mask[3];
            frame_offset++;
            for (long i = 0; i < frame->getDataLength(); i++) {
                frame_sequence[frame_offset] = frame->getData()[i] | mask[i % 4];
                frame_offset++;
            }
        } else {
            for (long i = 0; i < frame->getDataLength(); i++) {
                frame_sequence[frame_offset] = frame->getData()[i];
                frame_offset++;
            }
        }
        return std::string(frame_sequence, frame_offset);
    }
}