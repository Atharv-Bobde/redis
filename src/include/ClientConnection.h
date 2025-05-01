// File: clientConnection.h
//#pragma once is a non-standard but widely supported preprocessor directive that tells the compiler to include a header file only once per compilation unit, even if it’s included multiple times.
#pragma once
#include <thread>
#include <netdb.h>
#include <vector>
#include <unordered_map>
#include <functional>
using namespace std;

class ClientConnection {
    public:
        ClientConnection(int client_fd, sockaddr_in client_addr);
        void start();
        void join();
        ~ClientConnection();
    
    private:
        void handle(); 
        void handle_PING(vector<string> arr);
        void handle_ECHO(vector<string> arr);
        void handle_SET(vector<string> arr);
        void handle_GET(vector<string> arr);
        void handle_COMMAND(vector<string> arr);
        void handle_UNKNOWN(vector<string> arr);
        int client_fd_;
        sockaddr_in client_addr_;
        thread thread_;
        char client_ip[INET_ADDRSTRLEN];
        enum COMMANDS{
            PING,
            ECHO,
            SET,
            GET,
            COMMAND
        };
        unordered_map<string,COMMANDS> command_map = {
            {"PING", PING},
            {"ECHO",ECHO},
            {"SET", SET},
            {"GET", GET},
            {"COMMAND", COMMAND}
        };
        unordered_map<COMMANDS,function<void(vector<string>arr)>> command_handlers;
        unordered_map<string,string> dataMap; // map to store key-value pairs
};