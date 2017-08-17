#include <iostream>

#include "ClientHandler.hpp"

int main(int argc, char const *argv[]) {
  ClientHandler c;
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
      }
      case 0:
        return 0;
    }
  }
}
