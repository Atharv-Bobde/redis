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
using namespace std;
void parseRESPArray(const string& input,vector<string> &result) {

  stringstream ss(input);
  char type;
  ss >> type;

  if (type != '*') {
     cerr <<"Invalid RESP array format: expected '*', got '" + string(1, type) + "'";
  }

 
  int array_size;
  ss >> array_size;
  if (ss.fail() || array_size < 0) {
      cerr <<"Invalid RESP array size";
  }

  // Consume the newline character after reading the array size *2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n
  ss.ignore(numeric_limits<streamsize>::max(), '\n');
  char c =ss.peek();

  for (int64_t i = 0; i < array_size; ++i) {
      if (ss.peek() != '$') {
         cerr <<"Invalid RESP array element format: missing string indicator";
      }
      ss >> type; // Read '$'
       if (type != '$') {
         cerr <<"Invalid RESP array element format: expected '$', got '" + string(1, type) + "'";
      }

      int64_t string_length;
      ss >> string_length;
       if (ss.fail() || string_length < 0) {
         cerr <<"Invalid RESP string length";
      }
       // Consume the newline character after reading the string length
      ss.ignore(numeric_limits<streamsize>::max(), '\n');

      string element;
      element.resize(string_length);
      ss.read(&element[0], string_length);


      result.push_back(element);

      // Consume the newline character after reading the string data
       ss.ignore(numeric_limits<streamsize>::max(), '\n');
  }

}

string toRESPBulkStrings(const vector<string>& items,int start,int end) {
  string resp;
  for (int i = start; i < end; ++i) {
    const string& item = items[i];
      resp += "$" + to_string(item.size()) + "\r\n" + item + "\r\n";
  }
  return resp;
}

void parseBulkStrings(const string& input,vector<string> &result) {
  size_t pos = 0;

  while (pos < input.size()) {
      if (input[pos] == '$') {
          // Find end of length line
          size_t len_end = input.find("\r\n", pos);
          if (len_end == string::npos) break;

          // Parse length
          int length = stoi(input.substr(pos + 1, len_end - pos - 1));
          pos = len_end + 2;

          // Check bounds and extract the string
          if (pos + length > input.size()) break;
          string str = input.substr(pos, length);
          result.push_back(str);

          pos += length + 2; // Move past content and \r\n
      } else {
          break;
      }
  }
}
void handle_client(int client_fd) {
  string ping_response = "+PONG\r\n";
  char buffer[1024];
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
      if(arr.size() > 0 && arr[0]=="ECHO"){
        // ECHO command
        string echo_response = toRESPBulkStrings(arr,1,arr.size());
        write(client_fd, echo_response.c_str(), echo_response.size());
      }else if(arr.size() > 0 && arr[0]=="PING"){
        // PING command
        write(client_fd, ping_response.c_str(), ping_response.size());
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
