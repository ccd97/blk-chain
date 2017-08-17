#ifndef __ENCODER_DECODER__
#define __ENCODER_DECODER__

#include <cstring>

#include "message.h"


class EncoderDecoder {

  //Encoder

public:
  int encodeConnectMsg(auto& buffer, auto s_port_no, auto r_port_no){
    ConnectMessage msg;
    int msgSize = sizeof(ConnectMessage);
    msg.header.packetSize = msgSize;
    msg.header.msgType = MessageType::ConnectMsg;
    msg.header.senderPortNo = s_port_no;
    msg.receivePort = r_port_no;
    std::memcpy(buffer, &msg, sizeof(ConnectMessage));
    return msgSize;
  }

  unsigned short encodeConnectMsgAck(auto& buffer, auto port_no){
    auto* msg = (ConnectMessage*) buffer;
    unsigned short pport = msg->receivePort;
    msg->header.msgType = MessageType::ConnectAcknowledgementMsg;
    msg->receivePort = port_no;
    return pport;
  }

};

#endif
