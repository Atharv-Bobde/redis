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

void handle_client(int client_fd) {
  string ping_response = "+PONG\r\n";
  char buffer[1024];
  unordered_map<string,string> dataMap;
  while(true){
    ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
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
          if(arr[0] == "PING"){
            // PING command
            write(client_fd, ping_response.c_str(), ping_response.size());
          }else if(arr[0] == "ECHO"){
            string echo_response = toRESPBulkStrings(arr,1,arr.size());
            write(client_fd, echo_response.c_str(), echo_response.size());
          }else if(arr[0]=="SET"){
            // SET command
            if(arr.size() < 3){
              cerr << "Invalid number of arguments for SET command\n";
              break;
            }
            string key = arr[1];
            string value = arr[2];
            dataMap.insert({key,value});
            string set_response = "+OK\r\n";
            write(client_fd, set_response.c_str(), set_response.size());
            if(arr.size()==5){
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
          }else if(arr[0]=="GET"){
            // GET command
            string key = arr[1];
            if(dataMap.find(key) != dataMap.end()){
              string value = dataMap[key];
              string get_response = toRESPBulkStrings({value},0,1);
              write(client_fd, get_response.c_str(), get_response.size());
            }else{
              string get_response = "$-1\r\n";
              write(client_fd, get_response.c_str(), get_response.size());
            }
          }else if(arr[0]=="COMMAND"){
            send(client_fd, "*0\r\n", 4, 0);
          }
          else{
            cerr << "Unknown command: " << arr[0] << "\n";
          }
        }
  }
  close(client_fd);
}

int main(int argc, char **argv) {
  // Flush after every cout / cerr
  cout << unitbuf;
  cerr << unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   cerr << "Failed to create server socket\n";
   return 1;
  }
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  cout << "Waiting for a client to connect...\n";

  cout << "Logs from program will appear here!\n";
  vector<thread> client_threads;
  while(true){
    // use nc localhost 6379 to connect to the server
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << "\n";
    client_threads.emplace_back(thread(handle_client, client_fd)); // same as client_threads.push_back(thread(handle_client, client_fd)); but more efficient as we dont create a copy of the thread object
  }
  for (auto& t : client_threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  // Close the server socket
  close(server_fd);
  return 0;
}
