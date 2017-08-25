#include <iostream>
#include <signal.h>

#include "ClientHandler.hpp"

ClientHandler c;

int main(int argc, char const *argv[]) {
  
  c.start();

  while(true){
    int choice;
    std::cout<<"\n1.Print Client Ports \n2.Print BlockChain \n3.Add Data \n0.Exit \nEnter Choice:";
    std::cin>>choice;
    switch (choice) {
      case 1:
        c.printPeers();
        break;
      case 2:
        c.printBlockChain();
        break;
      case 3:{
        std::string data;
        std::cout<<"Enter Data :";
        std::cin>>data;
        c.addData(data);
        break;
      }
      default:
        break;
      case 0:
        c.disconnect();
        return 0;
    }
  }
}
