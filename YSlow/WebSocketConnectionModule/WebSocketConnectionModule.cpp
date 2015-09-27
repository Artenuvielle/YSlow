#include <sstream>
#include <ctime>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "../wsxx/WebsocketParser.h"
#include "../wsxx/base64.h"
#include "../httpxx/ResponseBuilder.hpp"
#include "../libyslow/Logger.h"
#include "WebSocketConnectionModule.h"
#include "sha1.h"

using namespace std;

char WeekdayAbbreviations[7][4] = {
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
    "Sun"
};

char MonthAbbreviations[12][4] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

char LongTimeNumbers[60][3] = {
    "00",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "17",
    "18",
    "19",
    "20",
    "21",
    "22",
    "23",
    "24",
    "25",
    "26",
    "27",
    "28",
    "29",
    "30",
    "31",
    "32",
    "33",
    "34",
    "35",
    "36",
    "37",
    "38",
    "39",
    "40",
    "41",
    "42",
    "43",
    "44",
    "45",
    "46",
    "47",
    "48",
    "49",
    "50",
    "51",
    "52",
    "53",
    "54",
    "55",
    "56",
    "57",
    "58",
    "59",
};

struct WebSocketConnectionModuleData {
    bool handshake_completed;
    http::BufferedRequest* request;
    wsxx::WebSocketParser* frame_parser;
};

WebScoketConnectionModule::WebScoketConnectionModule() {
}

WebScoketConnectionModule::~WebScoketConnectionModule() {
}

void WebScoketConnectionModule::handleRead(ConnectionModuleDataCarrier* socket, char* read_buffer, int bytes_read) {
    WebSocketConnectionModuleData* current_request_data = (WebSocketConnectionModuleData*)socket->getConnectionModuleData();
    if (current_request_data == nullptr) {
        current_request_data = new WebSocketConnectionModuleData{ false, nullptr, new wsxx::WebSocketParser() };
        socket->setConnectionModuleData(current_request_data);
    }
    if (current_request_data->handshake_completed) {
        Logger::debug << "Feeding " << bytes_read << " byte to wsxx" << endl;
        current_request_data->frame_parser->feed(read_buffer, bytes_read);
    } else {
        if (current_request_data->request == nullptr) {
            current_request_data->request = new http::BufferedRequest();
        }

        int parsed_bytes = 0;
        while (parsed_bytes < bytes_read) {
            int parsed_bytes_in_cycle = current_request_data->request->feed(read_buffer + parsed_bytes, bytes_read - parsed_bytes);
            parsed_bytes += parsed_bytes_in_cycle;
        }
    }
}

ProcessingPipelinePacket* WebScoketConnectionModule::getCompletePacket(ConnectionModuleDataCarrier* socket) {
    WebSocketConnectionModuleData* current_request_data = ((WebSocketConnectionModuleData*)socket->getConnectionModuleData());
    if (current_request_data == nullptr) {
        current_request_data = new WebSocketConnectionModuleData{ false, nullptr, new wsxx::WebSocketParser() };
        socket->setConnectionModuleData(current_request_data);
    }
    if (current_request_data->handshake_completed) {
        wsxx::WebSocketFrame* frame = current_request_data->frame_parser->getComplete();
        if (frame != nullptr) {
            if (frame->getOpcode() == wsxx::TYPE_TEXT_FRAME) {
                Logger::debug << "Real data: " << string(frame->getData(), frame->getDataLength()) << endl;

                stringstream request_stream;
                rapidjson::Document d;
                d.Parse(frame->getData());
                Logger::debug << "Is Array? " << (d.IsArray() ? "true" : "false") << endl;
                request_stream << d[0].GetString() << " " << d[1].GetString() << " HTTP/1.1\r\n";
                const rapidjson::Value& header_object = d[2];
                for (rapidjson::Value::ConstMemberIterator it = header_object.MemberBegin(); it != header_object.MemberEnd(); it++) {
                    request_stream << it->name.GetString() << ": " << it->value.GetString() << "\r\n";
                }
                request_stream << "\r\n";

                if (d.Size() > 3) {
                    request_stream << d[3].GetString();
                }

                http::BufferedRequest* http_handshake_request = new http::BufferedRequest();
                Logger::info << "Turned into:" << request_stream.str() << endl;
                http_handshake_request->feed(request_stream.str().c_str(), request_stream.str().length());

                ProcessingPipelinePacket* packet = new ProcessingPipelinePacket(PIPELINE_REQUEST);
                packet->setPacketRequestData(http_handshake_request);
                return packet;
            } else {
                // not supported yet
            }
        }
    } else {
        if (current_request_data->request == nullptr) {
            current_request_data->request = new http::BufferedRequest();
        }
        if (current_request_data->request->complete()) {
            ProcessingPipelinePacket* packet;
            if (current_request_data->request->header("Connection") == "Upgrade" && current_request_data->request->header("Upgrade") == "websocket" && current_request_data->request->header("Sec-WebSocket-Version") == "13") {
                stringstream response_stream;

                stringstream guid_stream;
                guid_stream << current_request_data->request->header("Sec-WebSocket-Key") << "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                unsigned char hash[20];
                sha1::calc(guid_stream.str().c_str(), guid_stream.str().length(), hash);

                time_t t = time(0);   // get time now
                struct tm* now = gmtime(&t);

                response_stream << "HTTP/" << current_request_data->request->major_version() << "." << current_request_data->request->minor_version() << " 101 Web Socket Protocol Handshake\r\n";
                response_stream << "Connection: Upgrade\r\n";
                response_stream << "Date: " << WeekdayAbbreviations[now->tm_wday] << ", " << now->tm_mday << " " << MonthAbbreviations[now->tm_mon] << " " << (now->tm_year + 1900) << " ";
                response_stream << LongTimeNumbers[now->tm_hour] << ":" << LongTimeNumbers[now->tm_min] << ":" << LongTimeNumbers[now->tm_sec] << " GMT" << "\r\n";
                response_stream << "Sec-WebSocket-Accept: " << base64_encode(hash, 20) << "\r\n";
                response_stream << "Server: YSlow-Proxy\r\n";
                response_stream << "Upgrade: websocket\r\n";
                response_stream << "\r\n";

                http::BufferedResponse* http_handshake_response = new http::BufferedResponse();
                http_handshake_response->feed(response_stream.str().c_str(), response_stream.str().length());

                packet = new ProcessingPipelinePacket(PIPELINE_RESPONSE);
                packet->setPacketResponseData(http_handshake_response);
            } else {
                packet = new ProcessingPipelinePacket(PIPELINE_REQUEST);
            }
            packet->setPacketRequestData(current_request_data->request);
            current_request_data->request = nullptr;
            return packet;
        }
    }
    return nullptr;
}

string WebScoketConnectionModule::handleWrite(ConnectionModuleDataCarrier* socket, ProcessingPipelinePacket* packet) {
    WebSocketConnectionModuleData* current_request_data = ((WebSocketConnectionModuleData*)socket->getConnectionModuleData());
    if (current_request_data->handshake_completed) {
        http::BufferedResponse* response = packet->getPacketResponseData();
        rapidjson::Document d(rapidjson::kArrayType);
        d.PushBack(response->status(), d.GetAllocator());
        rapidjson::Value headers(rapidjson::kObjectType);
        for (map<string, string>::const_iterator it = response->headers().begin(); it != response->headers().end(); it++) {
            rapidjson::Value first(it->first.c_str(), it->first.length(), d.GetAllocator());
            rapidjson::Value second(it->second.c_str(), it->second.length(), d.GetAllocator());
            headers.AddMember(first, second, d.GetAllocator());
        }
        d.PushBack(headers, d.GetAllocator());
        rapidjson::Value body(response->body().c_str(), response->body().length(), d.GetAllocator());
        d.PushBack(body, d.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        d.Accept(writer);
        string json_string = buffer.GetString();

        wsxx::WebSocketFrame response_frame;
        response_frame.setFin(true);
        response_frame.setRSV1(false);
        response_frame.setRSV2(false);
        response_frame.setRSV3(false);
        response_frame.setOpcode(wsxx::TYPE_TEXT_FRAME);
        response_frame.setDataLength(json_string.length());
        response_frame.setData((char*)json_string.c_str());

        wsxx::WebSocketFrameBuilder frame_builder(&response_frame);
        string s = frame_builder.toString(false);
        Logger::debug << "returning: " << s << endl;
        return s;
    } else {
        if (packet->getPacketType() == PIPELINE_RESPONSE) {
            http::BufferedResponse* response = packet->getPacketResponseData();
            if (response->header("Upgrade") == "websocket") {
                current_request_data->handshake_completed = true;
            }
            http::ResponseBuilder* responseBuilder = new http::ResponseBuilder(*response);
            string ret = responseBuilder->to_string() + response->body();
            delete responseBuilder;
            return ret;
        }
    }
    return string();
}