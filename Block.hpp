#ifndef __BLOCK_HPP__
#define __BLOCK_HPP__

#include <string>
#include <iomanip>
#include <thread>
#include <iostream>

#include <openssl/sha.h>


class Block{

  using Idx = unsigned long long int;
  using Nonce = unsigned long long int;

private:

  Idx index;
  std::string phash;
  std::string chash;
  Nonce nonce;
  std::string data;

public:

  Block(Idx _idx, auto& _phash, auto& _data) : index(_idx), phash(_phash), data(_data) {
    mine_block();
  }

  void mine_block(bool force=false){
    if(force) hash_it();
    while(!is_mined()){
      nonce++;
      hash_it();
    }
  }

  // Getters

  auto& get_chash(){
    return chash;
  }

  auto& get_phash(){
    return phash;
  }

  auto get_index(){
    return index;
  }

  auto& get_data(){
    return data;
  }

  // Setters

  void set_phash(auto&& _phash){
    phash = _phash;
  }

  void set_data(auto&& _data){
    data = _data;
  }

private:

  void hash_it(){
    chash = sha256(std::to_string(index) + phash + data + std::to_string(nonce));
  }

  std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
      ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
  }

  bool is_mined(){
    auto len = chash.length();
    if (chash[len - 1] == '4' && chash[len - 2] == '3' &&
        chash[len - 3] == '2' && chash[len - 4] == '1') {
      return true;
    } else {
      return false;
    }
  }

};

#endif
