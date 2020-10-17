#ifndef _SERVER_HPP
#define _SERVER_HPP

//C++ versions of C headers
#include <cstring>
#include <cctype>
#include <cmath>

//C++ headers
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <thread>

typedef int socket_id;

class Server {
  private:
    void registerSocket(unsigned int port, unsigned int connectionCount = 10);

    void handle();

    socket_id serverSocket;
    std::thread handlerThread;

  public:
    Server(unsigned int port = 80);

    void addHandle(std::string handle, void (*function)(void*));



};

#endif //_SERVER_HPP
