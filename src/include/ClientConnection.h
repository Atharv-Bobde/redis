// File: clientConnection.h
//#pragma once is a non-standard but widely supported preprocessor directive that tells the compiler to include a header file only once per compilation unit, even if it’s included multiple times.
#pragma once
#include <thread>
#include <netdb.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <queue>
using namespace std;

class ClientConnection {
    public:
        ClientConnection(int client_fd, sockaddr_in client_addr, string DIR,string FILENAME);
        void start();
        void join();
        ~ClientConnection();
        static unordered_map<string,string> dataMap; // map to store key-value pairs
    private:
        void handle(); 
        string handle_PING(vector<string> arr);
        string handle_ECHO(vector<string> arr);
        string handle_SET(vector<string> arr);
        string handle_GET(vector<string> arr);
        string handle_COMMAND(vector<string> arr);
        string handle_UNKNOWN(vector<string> arr);
        string handle_CONFIG_GET(vector<string> arr);
        string handle_INCR(vector<string> arr);
        string handle_MULTI(vector<string> arr);
        string handle_EXEC(vector<string> arr);
        string handle_DISCARD(vector<string> arr);
        int client_fd_;
        sockaddr_in client_addr_;
        thread thread_;
        char client_ip[INET_ADDRSTRLEN];
        enum COMMANDS{
            PING,
            ECHO,
            SET,
            GET,
            COMMAND,
            CONFIG,
            MULTI,
            EXEC,
            DISCARD,
            INCR,

        };
        unordered_map<string,COMMANDS> command_map = {
            {"PING", PING},
            {"ECHO",ECHO},
            {"SET", SET},
            {"GET", GET},
            {"COMMAND", COMMAND},
            {"CONFIG",CONFIG},
            {"MULTI",MULTI},
            {"EXEC",EXEC},
            {"DISCARD",DISCARD},
            {"INCR", INCR},
            
        };
        unordered_map<COMMANDS,function<string(vector<string>)>> command_handlers;
        string DIR_,FILENAME_;
        queue<pair<COMMANDS,vector<string>>> command_queue;
        bool transaction;
    };