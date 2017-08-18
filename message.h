#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "Block.hpp"

using Idx = unsigned long long int;

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
  char chash[HASH_SIZE];
};

struct RequestDataMessage{
  MessageHeader header;
  Idx idx;
  char chash[HASH_SIZE];
};

struct ResponseDataMessage{
  MessageHeader header;
  Idx idx;
  char data[DATA_SIZE];
};

struct chash_response {
  Idx idx;
  unsigned short port;
  std::string chash;
};

#endif
