#ifndef __PRACTICALSOCKET_HPP_INCLUDED__
#define __PRACTICALSOCKET_HPP_INCLUDED__

#include <string>            // For string
#include <exception>         // For exception class

#include <cstdlib>
#include <cstring>

#ifdef WIN32
  #include <winsock.h>         // For socket(), connect(), send(), and recv()
  typedef int socklen_t;
  typedef char raw_type;       // Type used for raw data on this platform
#else
  #include <sys/types.h>       // For data types
  #include <sys/socket.h>      // For socket(), connect(), send(), and recv()
  #include <netdb.h>           // For gethostbyname()
  #include <arpa/inet.h>       // For inet_addr()
  #include <unistd.h>          // For close()
  #include <netinet/in.h>      // For sockaddr_in
  typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

using namespace std;

using std::string;
using std::exception;

#ifdef WIN32
static bool initialized = false;
#endif


class SocketException : public exception {

public:

  SocketException(const string &message, bool inclSysMsg = false) throw() : userMessage(message) {
    if (inclSysMsg) {
      userMessage.append(": ");
      userMessage.append(strerror(errno));
    }
  }

  ~SocketException() throw() {}

  const char *what() const throw() {
    return userMessage.c_str();
  }

private:
  string userMessage;  // Exception message

};


// Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short port, sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr));  // Zero out address structure
  addr.sin_family = AF_INET;       // Internet address

  hostent *host;  // Resolve name
  if ((host = gethostbyname(address.c_str())) == NULL) {
    throw SocketException("Failed to resolve name (gethostbyname())");
  }
  addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

  addr.sin_port = htons(port);     // Assign port in network byte order
}


class Socket {

public:

  ~Socket() {
    #ifdef WIN32
      ::closesocket(sockDesc);
    #else
      ::close(sockDesc);
    #endif
    sockDesc = -1;
  }

  string getLocalAddress() {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
      throw SocketException("Fetch of local address failed (getsockname())", true);
    }
    return inet_ntoa(addr.sin_addr);
  }

  unsigned short getLocalPort()  {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
      throw SocketException("Fetch of local port failed (getsockname())", true);
    }
    return ntohs(addr.sin_port);
  }

  void setLocalPort(unsigned short localPort) {
    // Bind the socket to its port
    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(localPort);

    if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
      throw SocketException("Set of local port failed (bind())", true);
    }
  }

  void setLocalAddressAndPort(const string &localAddress, unsigned short localPort = 0) {
    // Get the address of the requested host
    sockaddr_in localAddr;
    fillAddr(localAddress, localPort, localAddr);

    int so_reuseaddr =1;
    setsockopt(sockDesc,SOL_SOCKET,SO_REUSEADDR,&so_reuseaddr,sizeof(so_reuseaddr));

    if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
      throw SocketException("Set of local address and port failed (bind())", true);
    }
  }

  static void cleanUp() {
    #ifdef WIN32
      if (WSACleanup() != 0) {
        throw SocketException("WSACleanup() failed");
      }
    #endif
  }

  static unsigned short resolveService(const string &service, const string &protocol = "tcp") {
    struct servent *serv;        /* Structure containing service information */

    if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL)
      return atoi(service.c_str());  /* Service is port number */
    else
      return ntohs(serv->s_port);    /* Found port (network byte order) by name */
  }

private:
  Socket(const Socket &sock);
  void operator=(const Socket &sock);

protected:

  int sockDesc;              // Socket descriptor

  Socket(int type, int protocol) {
    #ifdef WIN32
      if (!initialized) {
        WORD wVersionRequested;
        WSADATA wsaData;

        wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
        if (WSAStartup(wVersionRequested, &wsaData) != 0) {  // Load WinSock DLL
          throw SocketException("Unable to load WinSock DLL");
        }
        initialized = true;
      }
    #endif

    // Make a new socket
    if ((sockDesc = socket(PF_INET, type, protocol)) < 0) {
      throw SocketException("Socket creation failed (socket())", true);
    }

  }

  Socket(int sockDesc) {
    this->sockDesc = sockDesc;
  }

};


/**
 *   Socket which is able to connect, send, and receive
 */
class CommunicatingSocket : public Socket {

public:

  /**
   *   Establish a socket connection with the given foreign
   *   address and port
   *   @param foreignAddress foreign address (IP address or name)
   *   @param foreignPort foreign port
   *   @exception SocketException thrown if unable to establish connection
   */
  void connect(const string &foreignAddress, unsigned short foreignPort) {
    // Get the address of the requested host
    sockaddr_in destAddr;
    fillAddr(foreignAddress, foreignPort, destAddr);

    // Try to connect to the given port
    if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0) {
      throw SocketException("Connect failed (connect())", true);
    }
  }

  /**
   *   Write the given buffer to this socket.  Call connect() before
   *   calling send()
   *   @param buffer buffer to be written
   *   @param bufferLen number of bytes from buffer to be written
   *   @exception SocketException thrown if unable to send data
   */
  void send(const void *buffer, int bufferLen) {
    if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0) {
      throw SocketException("Send failed (send())", true);
    }
  }

  /**
   *   Read into the given buffer up to bufferLen bytes data from this
   *   socket.  Call connect() before calling recv()
   *   @param buffer buffer to receive the data
   *   @param bufferLen maximum number of bytes to read into buffer
   *   @return number of bytes read, 0 for EOF, and -1 for error
   *   @exception SocketException thrown if unable to receive data
   */
  int recv(void *buffer, int bufferLen, int flag=0) {
    int rtn;
    if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, flag)) < 0) {
      //throw SocketException("Received failed (recv())", true);
    }

    return rtn;
  }

  /**
   *   Get the foreign address.  Call connect() before calling recv()
   *   @return foreign address
   *   @exception SocketException thrown if unable to fetch foreign address
   */
  string getForeignAddress() {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0) {
      throw SocketException("Fetch of foreign address failed (getpeername())", true);
    }
    return inet_ntoa(addr.sin_addr);
  }

  /**
   *   Get the foreign port.  Call connect() before calling recv()
   *   @return foreign port
   *   @exception SocketException thrown if unable to fetch foreign port
   */
  unsigned short getForeignPort() {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
      throw SocketException("Fetch of foreign port failed (getpeername())", true);
    }
    return ntohs(addr.sin_port);
  }

protected:

  CommunicatingSocket(int type, int protocol) : Socket(type, protocol) {}
  CommunicatingSocket(int newConnSD) : Socket(newConnSD) {}

};


/**
 *   TCP socket for communication with other TCP sockets
 */
class TCPSocket : public CommunicatingSocket {

public:

  /**
   *   Construct a TCP socket with no connection
   *   @exception SocketException thrown if unable to create TCP socket
   */
  TCPSocket() : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {}

  /**
   *   Construct a TCP socket with a connection to the given foreign address
   *   and port
   *   @param foreignAddress foreign address (IP address or name)
   *   @param foreignPort foreign port
   *   @exception SocketException thrown if unable to create TCP socket
   */
  TCPSocket(const string &foreignAddress, unsigned short foreignPort) : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {
    connect(foreignAddress, foreignPort);
  }

private:

  // Access for TCPServerSocket::accept() connection creation
  friend class TCPServerSocket;

  TCPSocket(int newConnSD) : CommunicatingSocket(newConnSD) {}

};


/**
 *   TCP socket class for servers
 */
class TCPServerSocket : public Socket {

public:

  /**
   *   Construct a TCP socket for use with a server, accepting connections
   *   on the specified port on any interface
   *   @param localPort local port of server socket, a value of zero will
   *                   give a system-assigned unused port
   *   @param queueLen maximum queue length for outstanding
   *                   connection requests (default 5)
   *   @exception SocketException thrown if unable to create TCP server socket
   */
  TCPServerSocket(unsigned short localPort, int queueLen = 5) : Socket(SOCK_STREAM, IPPROTO_TCP) {
    setLocalPort(localPort);
    setListen(queueLen);
  }

  /**
   *   Construct a TCP socket for use with a server, accepting connections
   *   on the specified port on the interface specified by the given address
   *   @param localAddress local interface (address) of server socket
   *   @param localPort local port of server socket
   *   @param queueLen maximum queue length for outstanding
   *                   connection requests (default 5)
   *   @exception SocketException thrown if unable to create TCP server socket
   */
  TCPServerSocket(const string &localAddress, unsigned short localPort, int queueLen = 5) : Socket(SOCK_STREAM, IPPROTO_TCP) {
    setLocalAddressAndPort(localAddress, localPort);
    setListen(queueLen);
  }


  /**
   *   Blocks until a new connection is established on this socket or error
   *   @return new connection socket
   *   @exception SocketException thrown if attempt to accept a new connection fails
   */
  TCPSocket *accept() {
    int newConnSD;
    if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0) {
      throw SocketException("Accept failed (accept())", true);
    }

    return new TCPSocket(newConnSD);
  }

private:

  void setListen(int queueLen) {
    if (listen(sockDesc, queueLen) < 0) {
      throw SocketException("Set listening socket failed (listen())", true);
    }
  }

};


/**
  *   UDP socket class
  */
class UDPSocket : public CommunicatingSocket {

public:

  /**
   *   Construct a UDP socket
   *   @exception SocketException thrown if unable to create UDP socket
   */
  UDPSocket() : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
    setBroadcast();
  }

  /**
   *   Construct a UDP socket with the given local port
   *   @param localPort local port
   *   @exception SocketException thrown if unable to create UDP socket
   */
  UDPSocket(unsigned short localPort) : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
    setLocalPort(localPort);
    int optval = 1;
    setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    setBroadcast();
  }

  /**
   *   Construct a UDP socket with the given local port and address
   *   @param localAddress local address
   *   @param localPort local port
   *   @exception SocketException thrown if unable to create UDP socket
   */
  UDPSocket(const string &localAddress, unsigned short localPort) : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
    setLocalAddressAndPort(localAddress, localPort);
    //  setBroadcast();
    //int so_reuseaddr =1;
    //setsockopt(sockDesc,SOL_SOCKET,SO_REUSEADDR,&so_reuseaddr,sizeof(so_reuseaddr));
  }

  /**
   *   Unset foreign address and port
   *   @return true if disassociation is successful
   *   @exception SocketException thrown if unable to disconnect UDP socket
   */
  void disconnect() {
    sockaddr_in nullAddr;
    memset(&nullAddr, 0, sizeof(nullAddr));
    nullAddr.sin_family = AF_UNSPEC;

    // Try to disconnect
    if (::connect(sockDesc, (sockaddr *) &nullAddr, sizeof(nullAddr)) < 0) {
     #ifdef WIN32
      if (errno != WSAEAFNOSUPPORT) {
     #else
      if (errno != EAFNOSUPPORT) {
     #endif
        throw SocketException("Disconnect failed (connect())", true);
      }
    }
  }

  /**
   *   Send the given buffer as a UDP datagram to the
   *   specified address/port
   *   @param buffer buffer to be written
   *   @param bufferLen number of bytes to write
   *   @param foreignAddress address (IP address or name) to send to
   *   @param foreignPort port number to send to
   *   @return true if send is successful
   *   @exception SocketException thrown if unable to send datagram
   */
  void sendTo(const void *buffer, int bufferLen, const string &foreignAddress, unsigned short foreignPort) {
    sockaddr_in destAddr;
    fillAddr(foreignAddress, foreignPort, destAddr);

    // Write out the whole buffer as a single message.
    if (sendto(sockDesc, (raw_type *) buffer, bufferLen, 0, (sockaddr *) &destAddr, sizeof(destAddr)) != bufferLen) {
      throw SocketException("Send failed (sendto())", true);
    }
  }

  /**
   *   Read read up to bufferLen bytes data from this socket.  The given buffer
   *   is where the data will be placed
   *   @param buffer buffer to receive data
   *   @param bufferLen maximum number of bytes to receive
   *   @param sourceAddress address of datagram source
   *   @param sourcePort port of data source
   *   @return number of bytes received and -1 for error
   *   @exception SocketException thrown if unable to receive datagram
   */
  int recvFrom(void *buffer, int bufferLen, string &sourceAddress,
               unsigned short &sourcePort) {
    sockaddr_in clntAddr;
    socklen_t addrLen = sizeof(clntAddr);
    int rtn;
    if ((rtn = recvfrom(sockDesc, (raw_type *) buffer, bufferLen, 0, (sockaddr *) &clntAddr, (socklen_t *) &addrLen)) < 0) {
      throw SocketException("Receive failed (recvfrom())", true);
    }
    sourceAddress = inet_ntoa(clntAddr.sin_addr);
    sourcePort = ntohs(clntAddr.sin_port);

    return rtn;
  }

  /**
   *   Set the multicast TTL
   *   @param multicastTTL multicast TTL
   *   @exception SocketException thrown if unable to set TTL
   */
  void setMulticastTTL(unsigned char multicastTTL) {
    if (setsockopt(sockDesc, IPPROTO_IP, IP_MULTICAST_TTL, (raw_type *) &multicastTTL, sizeof(multicastTTL)) < 0) {
      throw SocketException("Multicast TTL set failed (setsockopt())", true);
    }
  }

  /**
   *   Join the specified multicast group
   *   @param multicastGroup multicast group address to join
   *   @exception SocketException thrown if unable to join group
   */
  void joinGroup(const string &multicastGroup) {
    struct ip_mreq multicastRequest;

    multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, (raw_type *) &multicastRequest, sizeof(multicastRequest)) < 0) {
      throw SocketException("Multicast group join failed (setsockopt())", true);
    }
  }

  /**
   *   Leave the specified multicast group
   *   @param multicastGroup multicast group address to leave
   *   @exception SocketException thrown if unable to leave group
   */
  void leaveGroup(const string &multicastGroup) {
    struct ip_mreq multicastRequest;

    multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, (raw_type *) &multicastRequest, sizeof(multicastRequest)) < 0) {
      throw SocketException("Multicast group leave failed (setsockopt())", true);
    }
  }

private:

  void setBroadcast() {
    // If this fails, we'll hear about it when we try to send.  This will allow
    // system that cannot broadcast to continue if they don't plan to broadcast
    int broadcastPermission = 1;
    setsockopt(sockDesc, SOL_SOCKET, SO_BROADCAST,
               (raw_type *) &broadcastPermission, sizeof(broadcastPermission));
  }

};


#endif
