#ifndef __MESSAGES_H__
#define __MESSAGES_H__

enum MessageType{
  ConnectMsg,
  ConnectAcknowledgementMsg
};

struct MessageHeader{
  unsigned int packetSize;
  MessageType msgType;
  unsigned short senderPortNo;
};

struct ConnectMessage{
  MessageHeader header;
  unsigned short receivePort;
};

#endif
