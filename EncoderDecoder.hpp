#ifndef __ENCODER_DECODER__
#define __ENCODER_DECODER__

#include <cstring>
#include <tuple>

#include "message.h"


class EncoderDecoder {

  //Encoder

public:
  int encodeConnectMsg(auto& buffer, auto s_port_no, auto r_port_no){
    ConnectMessage msg;
    int msgSize = sizeof(ConnectMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::ConnectMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    std::memcpy(buffer, &msg, sizeof(ConnectMessage));
    return msgSize;
  }

  unsigned short relayConnectAckMsg(auto& buffer, auto s_port_no, auto r_port_no){
    auto* msg = (ConnectMessage*) buffer;
    unsigned short pport = msg->header.receivePort;
    msg->header.msgType = MessageType::ConnectAcknowledgementMsg;
    msg->header.senderPort = s_port_no;
    msg->header.receivePort = r_port_no;
    return pport;
  }

  auto decodeRequestChashMsg(auto& buffer){
    auto* msg = (RequestChashMessage*) buffer;
    return std::tuple{msg->idx, msg->header.receivePort};
  }

  int encodeResponseHashMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index, auto hash){
    ResponseChashMessage msg;
    int msgSize = sizeof(ResponseChashMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::ResponseChashMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    msg.chash = hash;
    std::memcpy(buffer, &msg, sizeof(ResponseChashMessage));
    return msgSize;
  }

  int encodeResponseDataMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index, auto& data){
    ResponseDataMessage msg;
    int msgSize = sizeof(ResponseDataMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::ResponseDataMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    msg.data = data;
    std::memcpy(buffer, &msg, sizeof(ResponseDataMessage));
    return msgSize;
  }

  auto decodeResponseChashMsg(auto& buffer){
    auto* msg = (ResponseChashMessage*) buffer;
    return chash_response{msg->idx, msg->header.receivePort, msg->chash};
  }

  auto decodeRequestDataMsg(auto& buffer){
    auto* msg = (RequestDataMessage*) buffer;
    return std::tuple{msg->idx, msg->header.receivePort};
  }

  int encodeRequestDataMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index, auto hash){
    RequestDataMessage msg;
    int msgSize = sizeof(RequestDataMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::RequestDataMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    msg.chash = hash;
    std::memcpy(buffer, &msg, sizeof(RequestDataMessage));
    return msgSize;
  }

  int encodeRequestChashMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index){
    RequestChashMessage msg;
    int msgSize = sizeof(RequestChashMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::RequestChashMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    std::memcpy(buffer, &msg, sizeof(RequestChashMessage));
    return msgSize;
  }

};

#endif
