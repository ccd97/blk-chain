#ifndef __CLIENT_HANDLER_HPP__
#define __CLIENT_HANDLER_HPP__

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <thread>
#include <chrono>

#include "PracticalSocket.hpp"
#include "message.h"
#include "EncoderDecoder.hpp"
#include "log.h"

static constexpr auto START_PORT = 50000;
static constexpr auto END_PORT = 50100;
static constexpr auto BUFFER_SIZE = 1024*2;

static constexpr auto IP_ADDR = "127.0.0.1";

class ClientHandler{

private:

  std::unordered_set<unsigned short> peer_ports;

  unsigned short s_port;
  unsigned short r_port;
  UDPSocket *s_sock;
  UDPSocket *r_sock;

  EncoderDecoder encoder, decoder;

  char recvBuffer[BUFFER_SIZE];
  char sendBuffer[BUFFER_SIZE];

  std::thread listener_thread;

public:
  ClientHandler(){
    s_port = allocate_port(s_sock);
    r_port = allocate_port(r_sock);
    dmsg("Send Port : " << s_port);
    dmsg("Receive Port : " << r_port);
    sendConnectMessages();
    listener_thread = std::thread([this](){
      listen();
    });
    listener_thread.detach();
  }

  void printPeers(){
    for(auto i : peer_ports){
      std::cout << "Peer : " << i << std::endl;;
    }
  }

private:

  unsigned short allocate_port(auto& sock){
    std::srand(std::time(0));
    while(true){
      unsigned short rand_port = std::rand()%(END_PORT-START_PORT + 1) + START_PORT;
      try{
        dmsg("opening port on " << rand_port);
        sock = new UDPSocket(rand_port);
        return rand_port;
      }
      catch(SocketException &exp){
        err("failed to open port on " << rand_port);
        continue;
      }
    }
  }

  void sendConnectMessages(){
    for(int i=START_PORT; i<=END_PORT; i++){
      if(i == r_port) continue;
      s_sock->sendTo(sendBuffer, encoder.encodeConnectMsg(sendBuffer, s_port, r_port), IP_ADDR, i);
    }
  }

  void listen(){
    int totalRecvMsgSize = 0;
    while(true){
      totalRecvMsgSize = r_sock->recv(recvBuffer, BUFFER_SIZE, MSG_DONTWAIT);
      if(totalRecvMsgSize < 0 && errno != EAGAIN) {
          break; // Some error.
      }
      if(totalRecvMsgSize == -1 && errno == EAGAIN) {
          std::this_thread::sleep_for(std::chrono::microseconds(10));
          continue;
      }
      if(totalRecvMsgSize == 0) {
        break;
      }
      auto* messageHeader = (MessageHeader *)(recvBuffer);
      switch (messageHeader->msgType) {
        case MessageType::ConnectMsg:{
          unsigned short pport = encoder.encodeConnectMsgAck(recvBuffer, r_port);
          s_sock->sendTo(recvBuffer, sizeof(ConnectMessage), IP_ADDR, pport);
          peer_ports.insert(pport);
          break;
        }
        case MessageType::ConnectAcknowledgementMsg:{
          peer_ports.insert(((ConnectMessage*) recvBuffer)->receivePort);
          break;
        }
      }
    }

  }

};

#endif
