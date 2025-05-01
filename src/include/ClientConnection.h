// File: clientConnection.h
//#pragma once is a non-standard but widely supported preprocessor directive that tells the compiler to include a header file only once per compilation unit, even if it’s included multiple times.
#pragma once
#include <thread>
#include <netdb.h>
using namespace std;

class ClientConnection {
    public:
        ClientConnection(int client_fd, sockaddr_in client_addr);
        void start();
        void join();
        ~ClientConnection();
    
    private:
        void handle(); // private logic for handling client
    
        int client_fd_;
        sockaddr_in client_addr_;
        thread thread_;
        string ping_response = "+PONG\r\n";
        char client_ip[INET_ADDRSTRLEN];
};