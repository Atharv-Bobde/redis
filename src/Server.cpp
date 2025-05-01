#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include "RESP.utils.h"
#include "ClientConnection.h"
using namespace std;


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
  int bindResult = ::bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)); // :: is used to avoid ambiguity with the bind function in the current namespace
  // bind tells OS to associate the socket with the address and port
  if ( bindResult!= 0) {
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
  vector<ClientConnection*> client_connections;
  while(true){
    // use nc localhost 6379 to connect to the server or redis-cli
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    char client_ip[INET_ADDRSTRLEN];
    auto* conn= new ClientConnection(client_fd, client_addr);
    conn->start();
    client_connections.push_back(conn);
    // client_threads.emplace_back(thread(handle_client, client_fd)); // same as client_threads.push_back(thread(handle_client, client_fd)); but more efficient as we dont create a copy of the thread object
  }
  for (auto& t : client_connections) {
    t->join();
    delete t;
  }
  // Close the server socket
  close(server_fd);
  return 0;
}
