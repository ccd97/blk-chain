#ifndef __CLIENT_HANDLER_HPP__
#define __CLIENT_HANDLER_HPP__

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <vector>

#include "BlockChain/BlockChain.hpp"
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
    s_port = allocatePort(s_sock);
    r_port = allocatePort(r_sock);
    dmsg("Send Port : " << s_port);
    dmsg("Receive Port : " << r_port);
    sendConnectMessages();
    listener_thread = std::thread([this](){ listen(); });
    synchronizer = std::thread([this](){ startSyanchronizer(); });
  }

  void printPeers() const {
    for(auto i : peer_ports){
      std::cout << "Peer : " << i << std::endl;;
    }
  }

  void printBlockChain() const {
    bchain.printChain();
  }

  void addData(const std::string& data){
    bchain.addData(data);
  }

  void disconnect(){
    sendDisconnectMessage();
    s_sock->disconnect();
    r_sock->disconnect();
  }

private:

  unsigned short allocatePort(auto& sock){
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

  void sendDisconnectMessage(){
    int msgSize = encoder.encodeDisconnectMsg(sendBuffer, s_port, r_port);
    for(auto p: peer_ports){
      s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, p);
    }
  }

  void sendChashRequest(Idx index){
    // dmsg("Send Request Chash index:" << index);
    int msgSize = encoder.encodeRequestChashMsg(sendBuffer, s_port, r_port, index);
    for(auto p: peer_ports){
      // dmsg("Send Request Chash index:" << index << " port:" << p);
      s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, p);
    }
  }

  void sendDataRequest(Idx index, const std::string& chash, const std::vector<unsigned short>& ports){
    // dmsg("Send Request DATA index:" << index);
    int msgSize = encoder.encodeRequestDataMsg(sendBuffer, s_port, r_port, index, chash);
    for(auto p : ports){
      // dmsg("Send Request DATA index:" << index << " port:" << p);
      s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, p);
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

      const auto* const messageHeader = (MessageHeader *)(recvBuffer);

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

        case MessageType::DisconnectMsg:{
          unsigned short pport = encoder.decodeDisconnectMsg(recvBuffer);
          const auto p_it = peer_ports.find(pport);
          if(p_it != peer_ports.end()){
            peer_ports.erase(p_it);
          }
        }

        case MessageType::RequestChashMsg:{
          auto [index, send_to_port] = decoder.decodeRequestChashMsg(recvBuffer);
          // dmsg("Recv Request Chash index:" << index << " port:" << send_to_port);
          if(index < bchain.getLength()){
            auto requestedHash = bchain.getChash(index);
            auto msgSize = encoder.encodeResponseHashMsg(sendBuffer, s_port, r_port, index, requestedHash);
            // dmsg("Send Respons Chash : " << requestedHash);
            s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, send_to_port);
          }
          break;
        }

        case MessageType::ResponseChashMsg:{
          const auto res = decoder.decodeResponseChashMsg(recvBuffer);
          // dmsg("Recv Respons Chash index:" << res.idx << " hash:" << res.chash);
          chash_lfq.push(res);
          break;
        }

        case MessageType::RequestDataMsg:{
          auto [index, send_to_port] = decoder.decodeRequestDataMsg(recvBuffer);
          // dmsg("Recv Request DATA index:" << index << " port:" << send_to_port);
          if(index < bchain.getLength()){
            auto [nonce, phash, chash, data] = bchain.getBlock(index);
            auto msgSize = encoder.encodeResponseDataMsg(sendBuffer, s_port, r_port, index, nonce, phash, chash, data);
            s_sock->sendTo(sendBuffer, msgSize, IP_ADDR, send_to_port);
          }
          break;
        }

        case MessageType::ResponseDataMsg:{
          const auto& res = decoder.decodeResponseDataMsg(recvBuffer);
          // dmsg("Recv Reespons DATA index:" << res.idx << " data:" << res.data);
          bchain.updateBlock(res.idx, res.nonce, res.phash, res.chash, res.data);
          sendChashRequest(res.idx + 1);
          break;
        }
      }
    }

  }

  void startSyanchronizer(){

    sendChashRequest(0);

    Idx currIdx = -1;
    TimePoint now = std::chrono::system_clock::now();
    std::unordered_map<std::string, std::vector<unsigned short>> chash_response_map;

    while (true) {
      if(!chash_lfq.empty()){
        const auto* const res = chash_lfq.front<chash_response>();

        if(currIdx == -1){
          currIdx = res->idx;
          now = std::chrono::system_clock::now();
          chash_response_map.clear();
        }

        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count() > 500 || currIdx != res->idx){
          sendDataRequestFromChashResponse(currIdx, chash_response_map);
          currIdx = -1;
          continue;
        }

        if(auto c_it = chash_response_map.find(res->chash); c_it == chash_response_map.end()){
          chash_response_map.emplace(res->chash, std::vector{res->port});
        }
        else{
          c_it->second.push_back(res->port);
        }

        chash_lfq.pop(sizeof(chash_response));
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - now).count() > 100){
        if(currIdx != -1){
          sendDataRequestFromChashResponse(currIdx, chash_response_map);
        }
        currIdx = -1;
        now = std::chrono::system_clock::now();
        sendChashRequest(0);
      }

    }

  }

  void sendDataRequestFromChashResponse(const auto& currIdx, const auto& chash_response_map){
    int max = 0;
    std::string chash;
    const std::vector<unsigned short>* max_elem;
    for(const auto& p: chash_response_map){
      if(max < p.second.size()){
        max = p.second.size();
        max_elem = &p.second;
        chash = p.first;
      }
    }
    sendDataRequest(currIdx, chash, *max_elem);
  }

};

#endif
