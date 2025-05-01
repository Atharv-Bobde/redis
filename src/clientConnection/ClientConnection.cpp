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
ClientConnection::ClientConnection(int client_fd, sockaddr_in client_addr)
    : client_fd_(client_fd), client_addr_(client_addr) {
}

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
    unordered_map<string,string> dataMap;
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
        buffer[bytes_received] = '\0';
        string message(buffer);
        size_t pos=0;
        char type=message[0];
        vector<string> arr;
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
            if(command == "PING"){
              // PING command
              write(client_fd_, ping_response.c_str(), ping_response.size());
            }else if(command == "ECHO"){
              string echo_response = toRESPBulkStrings(arr,1,arr.size());
              cout<<"ECHO response: "<<echo_response.c_str()<<endl;
              write(client_fd_, echo_response.c_str(), echo_response.size());
            }else if(command=="SET"){
              // SET command
              if(arr.size() < 3){
                cerr << "Invalid number of arguments for SET command\n";
                break;
              }
              string key = arr[1];
              string value = arr[2];
              dataMap.insert({key,value});
              string set_response = "+OK\r\n";
              write(client_fd_, set_response.c_str(), set_response.size());
              if(arr.size()==5){
                string arg=arr[3];
                transform(arg.begin(), arg.end(), arg.begin(), ::toupper);
                if(arr[3]!="EX" && arr[3]!="PX"){
                  cerr << "Invalid argument for SET command\n";
                  break;
                }
                if(arr[3]=="EX"){
                  // set expiration time in seconds
                  int expiration_time = stoi(arr[4]);
                  thread([&dataMap,&key,expiration_time]() {
                    // Sleep for 5 seconds before erasing the key
                    this_thread::sleep_for(chrono::seconds(expiration_time));
            
                    // Erase the key from the map
                    if (dataMap.find(key) != dataMap.end()) {
                        cout << "Erasing key " << key << " from map after delay!" << endl;
                        dataMap.erase(key);
                    } else {
                        cout << "Key " << key << " not found in map!" << endl;
                    }
                  }).detach();
                }else if(arr[3]=="PX"){
                  // set expiration time in milliseconds
                  int expiration_time = stoi(arr[4]);
                  thread([&dataMap,&key,expiration_time]() {
                    // Sleep for 5 seconds before erasing the key
                    this_thread::sleep_for(chrono::milliseconds(expiration_time));
            
                    // Erase the key from the map
                    if (dataMap.find(key) != dataMap.end()) {
                        cout << "Erasing key " << key << " from map after delay!" << endl;
                        dataMap.erase(key);
                    } else {
                        cout << "Key " << key << " not found in map!" << endl;
                    }
                  }).detach();
                }
              }
            }else if(command=="GET"){
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
            }else if(command=="COMMAND"){
              send(client_fd_, "*0\r\n", 4, 0);
            }
            else{
              cerr << "Unknown command: " << command << "\n";
            }
          }
    }
    close(client_fd_);
}