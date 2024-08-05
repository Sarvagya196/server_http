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
#include <filesystem>
#include <fstream>

static std::string file_directory = "";

void handleClient(int client_fd){
  std::cout<<"Initializing the response...\n";
  // const char *message = "HTTP/1.1 200 OK\r\n\r\n";
  // if (send(client_fd, message, strlen(message), 0) < 0){
  //   std::cerr<<"Failed to send response...";
  //   return 1;
  // }

  char http_req[BUFSIZ];
  int bytes_size = read(client_fd, http_req, BUFSIZ);
  if (bytes_size < 0){
    std::cerr<<"Failed to read data...";
    return;
  }
  std::string ag(http_req);
  std::string content(http_req);
  //std::cout<<ag<<"\r\n";
  http_req[bytes_size] = '\0';
  char *comm = strtok(http_req, " ");
  std::string command(comm);

  if (command == "GET"){
    char *path = strtok(NULL, " ");

    std::string s(path);

    if (s == "/"){
      const char* message = "HTTP/1.1 200 OK\r\n\r\n";
      if (send(client_fd, message, strlen(message), 0) < 0){
        std::cerr<<"Failed to send response...";
        return;
      }
    }

    else if (s.find("/echo/") == 0){
      std::string str = s;
      str.erase(0,6);
      std::string response = "HTTP/1.1 200 OK\r\n";
      response += "Content-Type: text/plain\r\n";
      response += "Content-Length: ";
      response += std::to_string(str.length());
      response += "\r\n\r\n";
      response += str;
      response += "\r\n";
      std::cout<<response << std::endl;
      const char* message = response.c_str();
      if (send(client_fd, message, strlen(message), 0) < 0){
        std::cerr<<"Failed to send response...";
        return;
      }
    }
    
    else if (s.find("/user-agent") == 0){
      ag.erase(0, ag.find("User-Agent: "));
      int end = ag.find("\r\n")-12;
      std::string agent = ag.substr(12, end);
      std::string response = "HTTP/1.1 200 OK\r\n";
      response += "Content-Type: text/plain\r\n";
      response += "Content-Length: ";
      response += std::to_string(agent.length());
      response += "\r\n\r\n";
      response += agent;
      response += "\r\n";
      std::cout<<response<< std::endl;
      const char* message = response.c_str();
      if (send(client_fd, message, strlen(message), 0) < 0){
        std::cerr<<"Failed to send response...";
        return;
      }
    }
    
    else if (s.find("/files/") == 0 && file_directory != ""){
      std::string st = s;
      st.erase(0, 7);
      std::string full_path = file_directory.append("/").append(st);
      if (std::filesystem::exists(full_path)){
        std::ifstream file(full_path);
        if (!(file.is_open())){
          std::cerr<<"Error in reading file content...\n";
          return;
        }

        std::stringstream file_buf;
        file_buf << file.rdbuf();
        std::string file_content = file_buf.str();

        file.close();

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/octet-stream\r\n";
        response += "Content-Length: ";
        response += std::to_string(file_content.size());
        response += "\r\n\r\n";
        response += file_content;
        response += "\r\n";
        std::cout<<response << std::endl;
        const char* message = response.c_str();
        if (send(client_fd, message, strlen(message), 0) < 0){
          std::cerr<<"Failed to send response...";
          return;
        }
      }
      else{
        const char* message = "HTTP/1.1 404 Not Found\r\n\r\n";
        if (send(client_fd, message, strlen(message), 0) < 0){
          std::cerr<<"Failed to send response...";
          return;
        }
      }
    }

    else{
      const char* message = "HTTP/1.1 404 Not Found\r\n\r\n";
      if (send(client_fd, message, strlen(message), 0) < 0){
        std::cerr<<"Failed to send response...";
        return;
      }
    }
  }

  else if (command == "POST"){
    char *req = strtok(NULL, " ");
    std::string request(req);
    if (request.find("/files/") == 0){
      std::string cont = request;
      cont.erase(0,7);
      std::string fullPath = file_directory.append("/").append(cont);
      std::ofstream new_file(fullPath);
      if (!(new_file.is_open())){
        std::cerr<<"Error in creating file...\n";
        return;
      }
      std::string write_buff = content.substr(content.find_last_of("\n") + 1);
      new_file << write_buff.c_str();
      new_file.close();
      const char* message = "HTTP/1.1 201 Created\r\n\r\n";
      if (send(client_fd, message, strlen(message), 0) < 0){
        std::cerr<<"Failed to send response...";
        return;
      }
    }
    else{
      const char* message = "HTTP/1.1 404 Not Found\r\n\r\n";
      if (send(client_fd, message, strlen(message), 0) < 0){
        std::cerr<<"Failed to send response...";
        return;
      }
    }
  }

  close(client_fd);
  return;
}

int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!"<<std::endl;

  //Uncomment this block to pass the first stage
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  for (int i=0; i<argc; i++){
    if (static_cast<std::string>(argv[i]) == "--directory"){
      file_directory.assign(argv[i+1]);
      break;
    }
  }

  // if (argc > 2 && static_cast<std::string>(argv[1]) == "--directory") {
  //   file_directory.assign(argv[2]);
  // }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  std::vector<std::thread> clients;
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  while (true){
    std::cout << "Waiting for a client to connect..." << std::endl;
    
    // accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    // std::cout << "Client connected\n";
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd < 0){
      // std::cerr<<"Failed to accept client connection...\n";
      // return 1;
      break;
    }
    std::cout<<"Client Conncected..." << std::endl;
    clients.emplace_back(handleClient, client_fd);
  }

  for (auto &i : clients){
    i.join();
  }

  //close(client_fd);
  close(server_fd);

  return 0;
}

