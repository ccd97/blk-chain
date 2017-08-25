#ifndef __BLOCK_HPP__
#define __BLOCK_HPP__

#include <string>
#include <iomanip>

#include <openssl/sha.h>

#include "log.h"

static constexpr auto HASH_SIZE = 64;
static constexpr auto DATA_SIZE = 256;

using Idx = unsigned long long int;
using Nonce = unsigned long long int;


class Block{

private:

  Idx index;
  Nonce nonce;
  char phash[HASH_SIZE] = {0};
  char chash[HASH_SIZE] = {0};
  char data[DATA_SIZE] = {0};

public:

  Block(const Idx _idx, const std::string _phash, const std::string& _data) : index(_idx){
    if(_data.size() > DATA_SIZE){
      throw "Size of input data is more than block data section";
    }
    _phash.copy(phash, _phash.size());
    _data.copy(data, _data.size());
    mine_block();
  }

  Block(const Idx _idx, const Nonce _nonce, const std::string& _phash, const std::string& _chash, const std::string& _data): index(_idx), nonce(_nonce){
    _phash.copy(phash, _phash.size());
    _chash.copy(chash, _chash.size());
    _data.copy(data, _data.size());
  }

  void mine_block(bool force=false){
    if(force) hash_it();
    while(!is_mined()){
      nonce++;
      hash_it();
    }
  }

  // Getters

  const auto get_chash() const {
    return std::string(chash, HASH_SIZE);
  }

  const auto get_phash() const {
    return std::string(phash, HASH_SIZE);
  }

  const auto get_index() const {
    return index;
  }

  const auto get_nonce() const {
    return nonce;
  }

  const auto get_data() const {
    return std::string(data, DATA_SIZE);
  }

  // Setters

  void set_phash(auto&& _phash){
    _phash.copy(phash, HASH_SIZE);
  }

  void set_data(auto&& _data){
    _data.copy(data, DATA_SIZE);
  }

  void update_block(auto& _nonce, auto& _phash, auto& _chash, auto& _data){
    nonce = _nonce;
    _phash.copy(phash, HASH_SIZE);
    _chash.copy(chash, HASH_SIZE);
    _data.copy(data, DATA_SIZE);
  }

private:

  void hash_it(){
    auto hash = sha256(std::to_string(index) + std::string(phash) + std::string(data) + std::to_string(nonce));
    hash.copy(chash, hash.length());
  }

  std::string sha256(const std::string& str) const {
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

  bool is_mined() const {
    auto len = HASH_SIZE;
    if (chash[len - 1] == '4' && chash[len - 2] == '3' &&
        chash[len - 3] == '2' && chash[len - 4] == '1') {
      return true;
    } else {
      return false;
    }
  }

};

#endif
