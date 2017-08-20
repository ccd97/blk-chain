#ifndef __BLOCKCHAIN_HPP_
#define __BLOCKCHAIN_HPP_

#include <list>
#include <iostream>

#include "Block.hpp"

class BlockChain {

private:

  std::list<Block> chain;

public:

  BlockChain(){
    chain.emplace_back(0, "1234", "The Genisys Block");
    chain.back().mine_block();
  }

  void addData(std::string& data){
    chain.emplace_back(chain.size(), chain.back().get_chash(), data);
    chain.back().mine_block();
  }

  void updateBlock(auto _idx, auto _nonce, auto& _phash, auto& _chash, auto& _data){
    if(_idx == chain.size()){
      chain.emplace_back(_idx, _nonce, _phash, _chash, _data);
    }
    else if(_idx < chain.size()){
      auto c_it = chain.begin();
      std::advance(c_it, _idx);
      c_it->update_block(_nonce, _phash, _chash, _data);
    }
  }

  void updateData(auto _idx, auto& _data){
    auto c_it = chain.begin();
    std::advance(c_it, _idx);
    c_it->set_data(_data);
    c_it->mine_block(true);
    repairChain();
  }

  auto getLength(){
    return chain.size();
  }

  void repairChain(){
    auto f_it = chain.begin();
    auto s_it = ++chain.begin();
    while(s_it != chain.end()){
      if(s_it->get_phash() != f_it->get_chash()){
        s_it->set_phash(f_it->get_chash());
        s_it->mine_block();
      }
      ++f_it; ++s_it;
    }
  }

  auto getChash(auto index){
    auto c_it = chain.begin();
    std::advance(c_it, index);
    return c_it->get_chash();
  }

  auto getBlock(auto index){
    auto c_it = chain.begin();
    std::advance(c_it, index);
    return std::tuple{c_it->get_nonce(), c_it->get_phash(), c_it->get_chash(), c_it->get_data()};
  }

  void printChain(){
    for(auto& b : chain){
      std::cout<<"========== Block " << b.get_index() << " ==========" << std::endl;
      std::cout<<"P-hash : "<<b.get_phash()<<std::endl;
      std::cout<<"C-hash : "<<b.get_chash()<<std::endl;
      std::cout<<"Data : "<<b.get_data()<<std::endl;
      std::cout<<std::endl;
    }
  }

};

#endif
