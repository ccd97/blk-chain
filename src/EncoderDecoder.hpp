#ifndef __ENCODER_DECODER__
#define __ENCODER_DECODER__

#include <cstring>
#include <tuple>

#include "message.h"


class EncoderDecoder {

public:

  //Encoder

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

  int encodeDisconnectMsg(auto& buffer, auto s_port_no, auto r_port_no){
    DisconnectMessage msg;
    int msgSize = sizeof(DisconnectMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::DisconnectMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    std::memcpy(buffer, &msg, sizeof(DisconnectMessage));
    return msgSize;
  }

  int encodeResponseHashMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index, auto hash){
    ResponseChashMessage msg;
    int msgSize = sizeof(ResponseChashMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::ResponseChashMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    hash.copy(msg.chash, HASH_SIZE);
    std::memcpy(buffer, &msg, sizeof(ResponseChashMessage));
    return msgSize;
  }

  int encodeResponseDataMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index, auto nonce, const auto& phash, const auto& chash, const auto& data){
    ResponseDataMessage msg;
    int msgSize = sizeof(ResponseDataMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::ResponseDataMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    msg.nonce = nonce;
    phash.copy(msg.phash, HASH_SIZE);
    chash.copy(msg.chash, HASH_SIZE);
    data.copy(msg.data, DATA_SIZE);
    std::memcpy(buffer, &msg, sizeof(ResponseDataMessage));
    return msgSize;
  }

  int encodeRequestDataMsg(auto& buffer, auto s_port_no, auto r_port_no, auto index, auto hash){
    RequestDataMessage msg;
    int msgSize = sizeof(RequestDataMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::RequestDataMsg;
    msg.header.senderPort = s_port_no;
    msg.header.receivePort = r_port_no;
    msg.idx = index;
    hash.copy(msg.chash, HASH_SIZE);
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

  // Decoder

  const auto decodeDisconnectMsg(const auto& buffer){
    const auto* const msg = (DisconnectMessage*) buffer;
    return msg->header.receivePort;
  }

  const auto decodeRequestChashMsg(const auto& buffer){
    const auto* const msg = (RequestChashMessage*) buffer;
    return std::tuple{msg->idx, msg->header.receivePort};
  }

  const auto decodeResponseChashMsg(const auto& buffer){
    const auto* const msg = (ResponseChashMessage*) buffer;
    return chash_response{msg->idx, msg->header.receivePort, std::string(msg->chash)};
  }

  const auto decodeRequestDataMsg(const auto& buffer){
    const auto* const msg = (RequestDataMessage*) buffer;
    return std::tuple{msg->idx, msg->header.receivePort};
  }

  const auto decodeResponseDataMsg(const auto& buffer){
    const auto* const msg = (ResponseDataMessage*) buffer;
    return data_response{msg->idx, msg->header.receivePort, msg->nonce, std::string(msg->phash), std::string(msg->chash), std::string(msg->data)};
  }

  // Relay

  unsigned short relayConnectAckMsg(auto& buffer, auto s_port_no, auto r_port_no){
    auto* msg = (ConnectMessage*) buffer;
    unsigned short pport = msg->header.receivePort;
    msg->header.msgType = MessageType::ConnectAcknowledgementMsg;
    msg->header.senderPort = s_port_no;
    msg->header.receivePort = r_port_no;
    return pport;
  }

};

#endif
