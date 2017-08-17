#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>

using Idx = unsigned long long int;
using Hash = std::string;

enum MessageType{
  ConnectMsg,
  ConnectAcknowledgementMsg,
  RequestChashMsg,
  ResponseChashMsg,
  RequestDataMsg,
  ResponseDataMsg
};

struct MessageHeader{
  unsigned int packetSize;
  MessageType msgType;
  unsigned short senderPort;
  unsigned short receivePort;
};

struct ConnectMessage{
  MessageHeader header;
};

struct RequestChashMessage{
  MessageHeader header;
  Idx idx;
};

struct ResponseChashMessage{
  MessageHeader header;
  Idx idx;
  Hash chash;
};

struct RequestDataMessage{
  MessageHeader header;
  Idx idx;
  Hash chash;
};

struct ResponseDataMessage{
  MessageHeader header;
  Idx idx;
  std::string data;
};

struct chash_response {
  Idx idx;
  unsigned short port;
  std::string chash;
};

#endif
