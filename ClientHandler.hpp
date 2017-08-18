#ifndef __CLIENT_HANDLER_HPP__
#define __CLIENT_HANDLER_HPP__

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <vector>

#include "BlockChain.hpp"
#include "PracticalSocket.hpp"
#include "message.h"
#include "EncoderDecoder.hpp"
#include "log.h"
#include "LockFreeQueue.hpp"

static constexpr auto START_PORT = 50000;
static constexpr auto END_PORT = 50100;
static constexpr auto BUFFER_SIZE = 1024*2;
static constexpr auto CHASH_QUEUE_SIZE = 1024 * 8;

static constexpr auto IP_ADDR = "127.0.0.1";

using Idx = unsigned long long int;

class ClientHandler{

  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<Clock>;

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
  std::thread synchronizer;

  BlockChain bchain;

  LockFreeQueue<CHASH_QUEUE_SIZE, 1024> chash_lfq;


public:
  ClientHandler(){
    s_port = allocate_port(s_sock);
    r_port = allocate_port(r_sock);
    dmsg("Send Port : " << s_port);
    dmsg("Receive Port : " << r_port);
    sendConnectMessages();
    listener_thread = std::thread([this](){ listen(); });
    synchronizer = std::thread([this](){ start_synchronizer(); });
    // listener_thread.detach();
  }

  void printPeers(){
    for(auto i : peer_ports){
      std::cout << "Peer : " << i << std::endl;;
    }
  }

  void printBlockChain(){
    bchain.printChain();
  }

  void addData(std::string& data){
    bchain.addData(data);
  }

private:

  unsigned short allocate_port(auto& sock){
    std::srand(std::time(0));
    while(true){
      unsigned short rand_port = std::rand()%(END_PORT-START_PORT + 1) + START_PORT;
      try{
        dmsg("acquiring socket on " << rand_port);
        sock = new UDPSocket(rand_port);
        return rand_port;
      }
      catch(SocketException &exp){
        err("failed to acquire socket on port " << rand_port);
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

  void sendChashRequest(Idx index){
    int msgSize = encoder.encodeRequestChashMsg(sendBuffer, s_port, r_port, index);
    for(auto p: peer_ports){
      // err("Sending chash request for index:" << index << " to port:" << p);
      s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, p);
    }
  }

  void sendDataRequest(Idx index, std::string chash, std::vector<unsigned short> ports){
    int msgSize = encoder.encodeRequestDataMsg(sendBuffer, s_port, r_port, index, chash);
    for(auto p : ports){
      // err("Sending DATA request for index:" << index << " to port:" << p);
      s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, p);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100000));
    sendChashRequest(++index);
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
          unsigned short pport = encoder.relayConnectAckMsg(recvBuffer, s_port, r_port);
          s_sock->sendTo(recvBuffer, sizeof(ConnectMessage), IP_ADDR, pport);
          peer_ports.insert(pport);
          break;
        }

        case MessageType::ConnectAcknowledgementMsg:{
          peer_ports.insert(((ConnectMessage*) recvBuffer)->header.receivePort);
          break;
        }

        case MessageType::RequestChashMsg:{
          auto [index, send_to_port] = decoder.decodeRequestChashMsg(recvBuffer);
          // err("Received chash message request for index:" << index << " to port:" << send_to_port);
          if(index < bchain.getLength()){
            auto requestedHash = bchain.getChash(index);
            auto msgSize = encoder.encodeResponseHashMsg(sendBuffer, s_port, r_port, index, requestedHash);
            // err("Sending response hash : " << requestedHash);
            s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, send_to_port);
          }
          break;
        }

        case MessageType::ResponseChashMsg:{
          auto res = decoder.decodeResponseChashMsg(recvBuffer);
          // err("Received chash message response for index:" << res.idx << " with hash:" << res.chash);
          chash_lfq.push(res);
          break;
        }

        case MessageType::RequestDataMsg:{
          auto [index, send_to_port] = decoder.decodeRequestDataMsg(recvBuffer);
          // err("Received DATA message request for index:" << index << " to port:" << send_to_port);
          if(index < bchain.getLength()){
            auto&& data = bchain.getData(index);
            auto msgSize = encoder.encodeResponseDataMsg(sendBuffer, s_port, r_port, index, data);
            s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, send_to_port);
          }
          break;
        }
      }
    }

  }

  void start_synchronizer(){

    // dmsg("Starting synchronizer");

    sendChashRequest(0);

    Idx currIdx = -1;
    TimePoint now = std::chrono::system_clock::now();
    std::unordered_map<std::string, std::vector<unsigned short>> chash_response_map;

    while (true) {
      if(!chash_lfq.empty()){
        // err("Got something in chash_lfq");
        const chash_response* res = chash_lfq.front<chash_response>();

        if(currIdx == -1){
          currIdx = res->idx;
          now = std::chrono::system_clock::now();
          chash_response_map.clear();
        }

        // error(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count());

        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count() > 500 || currIdx != res->idx){
          int max = 0;
          std::string chash;
          std::vector<unsigned short>* max_elem;
          for(auto& p: chash_response_map){
            if(max < p.second.size()){
              max = p.second.size();
              max_elem = &p.second;
              chash = p.first;
            }
          }
          sendDataRequest(currIdx, chash, *max_elem);
          currIdx = -1;
          continue;
        }

        auto c_it = chash_response_map.find(res->chash);

        if(c_it == chash_response_map.end()){
          chash_response_map.emplace(res->chash, std::vector{res->port});
        }
        else{
          c_it->second.push_back(res->port);
        }

        chash_lfq.pop(sizeof(chash_response));
      }

      if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - now).count() > 10){
        currIdx = -1;
        sendChashRequest(0);
      }

    }

  }

};

#endif
