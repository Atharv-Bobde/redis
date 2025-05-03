#include "ClientConnection.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <cctype>
#include "RESP.utils.h"
#include <unordered_map>
using namespace std;
// Constructor to initialize the client connection
ClientConnection::ClientConnection(int client_fd, sockaddr_in client_addr,string DIR,string FILENAME)
    : client_fd_(client_fd), client_addr_(client_addr),DIR_(DIR),FILENAME_(FILENAME) {
      command_handlers  = {
        {PING,  [this](vector<string> arr){ handle_PING(arr);} },
        {ECHO, [this](vector<string> arr){ handle_ECHO(arr);} },
        {SET, [this](vector<string> arr){ handle_SET(arr);} },
        {GET,  [this](vector<string> arr){ handle_GET(arr);} },
        {COMMAND,  [this](vector<string> arr){ handle_COMMAND(arr);} },
        {CONFIG, [this](vector<string> arr){ handle_CONFIG_GET(arr);}}
    };
}
unordered_map<string,string> ClientConnection::dataMap; // map to store key-value pairs
// Destructor to close the client socket
ClientConnection::~ClientConnection() {
    if (client_fd_ >= 0) close(client_fd_);
}

void ClientConnection::start() {
    thread_ = std::thread(&ClientConnection::handle, this);
}
void ClientConnection::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
} 

// Function to handle client connection
void ClientConnection::handle(){
    
    inet_ntop(AF_INET, &client_addr_.sin_addr, client_ip, sizeof(client_ip));
    cout << "Client connected from " << client_ip << ":" << ntohs(client_addr_.sin_port) << "\n";
    char buffer[1024];
    while(true){
      ssize_t bytes_received = read(client_fd_, buffer, sizeof(buffer) - 1);
      // bytes_received=0 means the client has closed the connection  (could be after sending the message)
      // bytes_received<0 means an error occurred
      // bytes_received>0 means we received a message from the client
      if (bytes_received <= 0) {
        if (bytes_received == 0) {
          cout << "Client disconnected\n";
        } else {
          cerr << "Failed to receive data from client\n";
        }
        break;
      } 
      vector<string> arr;
        buffer[bytes_received] = '\0';
        string message(buffer);
        size_t pos=0;
        char type=message[0];        
        switch (type){
          case '*': // Array
            parseRESPArray(message,arr);
            break;
          case '$': // bulk String
            parseBulkStrings(message,arr);
            break;
        }
        if(arr.size() > 0){
            string command = arr[0];
            transform(command.begin(), command.end(), command.begin(), ::toupper);
            auto it = command_map.find(command);
            if (it != command_map.end()) {
                COMMANDS cmd = it->second;
                auto handler_it = command_handlers.find(cmd);
                if (handler_it != command_handlers.end()) {
                   handler_it->second(arr); // Call the corresponding handler
                } else {
                    handle_UNKNOWN(arr);
                }
            } else {
                handle_UNKNOWN(arr);
            }
          }
    }
    close(client_fd_);
}

void ClientConnection::handle_PING(vector<string> arr) {
    string ping_response = "+PONG\r\n";
    write(client_fd_, ping_response.c_str(), ping_response.size());
}

void ClientConnection::handle_ECHO(vector<string> arr) {
    string echo_response = toRESPBulkStrings(arr,1,arr.size());
    write(client_fd_, echo_response.c_str(), echo_response.size());
}

void ClientConnection::handle_SET(vector<string> arr) {
    // SET command logic
    if(arr.size() < 3){
      cerr << "Invalid number of arguments for SET command\n";
      return;
    }
    string key = arr[1];
    string value = arr[2];
    dataMap[key] = value; 
    string set_response = "+OK\r\n";
    write(client_fd_, set_response.c_str(), set_response.size());
    if(arr.size()==5){
      string arg=arr[3];
      transform(arg.begin(), arg.end(), arg.begin(), ::toupper);
      if(arr[3]!="EX" && arr[3]!="PX"){
        cerr << "Invalid argument for SET command\n";
        return;
      }

      int expiration_time = stoi(arr[4]);

      if(arr[3]=="EX"){
        expiration_time*=1000; // convert seconds to milliseconds
      }
      thread([this,key,expiration_time]() {
        // Sleep for the specified expiration time
        this_thread::sleep_for(chrono::milliseconds(expiration_time));

        if (dataMap.find(key) != dataMap.end()) {
            cout << "Erasing key " << key << " from map after delay!" << endl;
            dataMap.erase(key);
        } else {
            cout << "Key " << key << " not found in map!" << endl;
        }
      }).detach(); // detach the thread to allow it to run independently (asynchronously)
    }
}
void ClientConnection::handle_GET(vector<string> arr) {
  // GET command
  string key = arr[1];
  if(dataMap.find(key) != dataMap.end()){
    string value = dataMap[key];
    string get_response = toRESPBulkStrings({value},0,1);
    write(client_fd_, get_response.c_str(), get_response.size());
  }else{
    string get_response = "$-1\r\n";
    write(client_fd_, get_response.c_str(), get_response.size());
  }
}

void ClientConnection::handle_COMMAND(vector<string> arr) {
  // COMMAND command logic
  string command_response = "*0\r\n";
  write(client_fd_, command_response.c_str(), command_response.size());
}
 
void ClientConnection::handle_UNKNOWN(vector<string> arr) {
  // Handle unknown command
  string unknown_response = "-ERR Unknown command\r\n";
  write(client_fd_, unknown_response.c_str(), unknown_response.size());
}

void ClientConnection::handle_CONFIG_GET(vector<string>arr){
  string res="";
  if(arr[2]=="dir"){
    res=toRESPArray({arr[2],DIR_});
  }else if(arr[2]=="dbfilename"){
    res=toRESPArray({arr[2],FILENAME_});
  }
  write(client_fd_, res.c_str(), res.size());
}