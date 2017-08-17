#include <iostream>

#include "BlockChain.hpp"
#include "ClientHandler.hpp"

int main(int argc, char const *argv[]) {
  ClientHandler c;
  while(true){
    int choice;
    std::cout<<"\n1.Print Client Ports \n2.Exit \nEnter Choice:";
    std::cin>>choice;
    if(choice == 1){
      c.printPeers();
    }
    else if(choice == 2){
      break;
    }
  }
  return 0;
}
