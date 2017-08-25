#ifndef __CLIENT_HANDLER_HPP__
#define __CLIENT_HANDLER_HPP__

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <queue>

#include "BlockChain/BlockChain.hpp"
#include "PracticalSocket.hpp"
#include "message.h"
#include "EncoderDecoder.hpp"
#include "log.h"

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

  char recvBuffer[BUFFER_SIZE];
  char sendBuffer[BUFFER_SIZE];

  std::mutex send_mutex;
  std::mutex bchain_mutex;
  std::mutex peer_mutex;
  std::mutex chash_q_mutex;

  EncoderDecoder encoder, decoder;

  std::thread listener_thread;
  std::thread synchronizer;

  BlockChain bchain;

  std::queue<chash_response> chash_queue;

public:
  ClientHandler() {
    s_port = allocatePort(s_sock);
    r_port = allocatePort(r_sock);
    dmsg("Send Port : " << s_port);
    dmsg("Receive Port : " << r_port);
  }

  void start(){
    sendConnectMessages();
    listener_thread = std::thread([this](){ listen(); });
    synchronizer = std::thread([this](){ startSyanchronizer(); });
  }

  void printPeers() {
    std::scoped_lock peer_lock(peer_mutex);
    for(auto i : peer_ports){
      std::cout << "Peer : " << i << std::endl;;
    }
  }

  void printBlockChain() {
    std::scoped_lock bchain_lock(bchain_mutex);
    bchain.printChain();
  }

  void addData(const std::string& data){
    std::scoped_lock bchain_lock(bchain_mutex);
    bchain.addData(data);
  }

  void disconnect(){
    sendDisconnectMessage();
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

  void send(unsigned short foreignPort, auto encoding_fn){
    std::scoped_lock send_lock(send_mutex);
    int bufferLen = encoding_fn(sendBuffer);
    s_sock->sendTo(sendBuffer, bufferLen, IP_ADDR, foreignPort);
  }

  void sendMultiple(auto& foreignPorts, auto encoding_fn){
    std::scoped_lock send_lock(send_mutex);
    int bufferLen = encoding_fn(sendBuffer);
    for(auto port: foreignPorts){
      s_sock->sendTo(sendBuffer, bufferLen, IP_ADDR, port);
    }
  }

  void sendConnectMessages(){
    for(int i=START_PORT; i<=END_PORT; i++){
      if(i == r_port) continue;
      send(i, [this](auto& buffer){ return encoder.encodeConnectMsg(buffer, s_port, r_port); });
    }
  }

  void sendDisconnectMessage(){
    std::scoped_lock peer_lock(peer_mutex);
    sendMultiple(peer_ports, [this](auto& buffer){ return encoder.encodeDisconnectMsg(buffer, s_port, r_port); });
  }

  void sendChashRequest(Idx index){
    std::scoped_lock peer_lock(peer_mutex);
    sendMultiple(peer_ports, [this, index](auto& buffer){ return encoder.encodeRequestChashMsg(buffer, s_port, r_port, index); });
  }

  void sendDataRequest(Idx index, const std::string& chash, const std::vector<unsigned short>& ports){
    sendMultiple(ports, [this, index, &chash](auto& buffer){ return encoder.encodeRequestDataMsg(buffer, s_port, r_port, index, chash); });
  }

  void listen(){
    int totalRecvMsgSize = 0;

    while(true){

      totalRecvMsgSize = r_sock->recv(recvBuffer, BUFFER_SIZE, MSG_DONTWAIT);
      if(totalRecvMsgSize < 0 && errno != EAGAIN) {
          break; // Some error.
      }
      if(totalRecvMsgSize == -1 && errno == EAGAIN) {
          std::this_thread::sleep_for(std::chrono::microseconds(1000));
          continue;
      }
      if(totalRecvMsgSize == 0) {
        break;
      }

      const auto* const messageHeader = (MessageHeader *)(recvBuffer);

      switch (messageHeader->msgType) {

        case MessageType::ConnectMsg:{
          unsigned short pport = encoder.decodeConnectMsg(recvBuffer);
          send(pport, [this](auto& buffer){ return encoder.encodeConnectAckMsg(buffer, s_port, r_port); });
          std::scoped_lock peer_lock(peer_mutex);
          peer_ports.insert(pport);
          break;
        }

        case MessageType::ConnectAcknowledgementMsg:{
          unsigned short pport = encoder.decodeConnectAckMsg(recvBuffer);
          std::scoped_lock peer_lock(peer_mutex);
          peer_ports.insert(pport);
          break;
        }

        case MessageType::DisconnectMsg:{
          unsigned short pport = encoder.decodeDisconnectMsg(recvBuffer);
          std::scoped_lock peer_lock(peer_mutex);
          const auto p_it = peer_ports.find(pport);
          if(p_it != peer_ports.end()){
            peer_ports.erase(p_it);
          }
        }

        case MessageType::RequestChashMsg:{
          auto [index, send_to_port] = decoder.decodeRequestChashMsg(recvBuffer);
          // dmsg("Recv Request Chash index:" << index << " port:" << send_to_port);
          std::scoped_lock bchain_lock(bchain_mutex);
          if(index < bchain.getLength()){
            auto requestedHash = bchain.getChash(index);
            send(send_to_port, [this, index, requestedHash](auto& buffer){ return encoder.encodeResponseHashMsg(buffer, s_port, r_port, index, requestedHash); });
          }
          break;
        }

        case MessageType::ResponseChashMsg:{
          const auto res = decoder.decodeResponseChashMsg(recvBuffer);
          // dmsg("Recv Respons Chash index:" << res.idx << " hash:" << res.chash);
          std::scoped_lock chash_q_lock(chash_q_mutex);
          chash_queue.push(res);
          break;
        }

        case MessageType::RequestDataMsg:{
          auto [index, send_to_port] = decoder.decodeRequestDataMsg(recvBuffer);
          // dmsg("Recv Request DATA index:" << index << " port:" << send_to_port);
          std::scoped_lock bchain_lock(bchain_mutex);
          if(index < bchain.getLength()){
            auto [nonce, phash, chash, data] = bchain.getBlock(index);
            send(send_to_port, [this, index, nonce, &phash, &chash, &data](auto& buffer){ return encoder.encodeResponseDataMsg(buffer, s_port, r_port, index, nonce, phash, chash, data); });
          }
          break;
        }

        case MessageType::ResponseDataMsg:{
          const auto& res = decoder.decodeResponseDataMsg(recvBuffer);
          // dmsg("Recv Reespons DATA index:" << res.idx << " data:" << res.data);
          std::scoped_lock bchain_lock(bchain_mutex);
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
      chash_q_mutex.lock();
      while(!chash_queue.empty()){
        const chash_response res = chash_queue.front();

        if(currIdx == -1){
          currIdx = res.idx;
          now = std::chrono::system_clock::now();
          chash_response_map.clear();
        }

        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count() >  2500 || currIdx != res.idx){
          sendDataRequestFromChashResponse(currIdx, chash_response_map);
          currIdx = -1;
          continue;
        }

        if(auto c_it = chash_response_map.find(res.chash); c_it == chash_response_map.end()){
          chash_response_map.emplace(res.chash, std::vector{res.port});
        }
        else{
          c_it->second.push_back(res.port);
        }

        chash_queue.pop();
      }
      chash_q_mutex.unlock();

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - now).count() > 500){
        if(currIdx != -1){
          sendDataRequestFromChashResponse(currIdx, chash_response_map);
        }
        currIdx = -1;
        now = std::chrono::system_clock::now();
        sendChashRequest(0);
      }

    }

    std::this_thread::sleep_for(std::chrono::microseconds(1000));

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
