#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
std::vector<int> clients;
std::map<int,std::string> users;
std::mutex mtx;

void broadcast(std::string msg,int sender=-1){
 std::lock_guard<std::mutex> lock(mtx);
 for(int c:clients) if(c!=sender) send(c,msg.c_str(),msg.size(),0);
}

void handle(int sock){
 char buffer[1024];
 recv(sock,buffer,1024,0);
 std::string name(buffer);
 users[sock]=name;
 broadcast("[SERVER] "+name+" joined\n");

 while(true){
  memset(buffer,0,1024);
  int n=recv(sock,buffer,1024,0);
  if(n<=0) break;
  std::string msg(buffer);

  if(msg.substr(0,5)=="/nick"){
    users[sock]=msg.substr(6);
  } else if(msg.substr(0,4)=="/msg"){
    // simplified
  } else {
    broadcast("["+users[sock]+"]: "+msg,sock);
  }
 }
 close(sock);
}

int main(){
 int s=socket(AF_INET,SOCK_STREAM,0);
 sockaddr_in addr{};
 addr.sin_family=AF_INET;
 addr.sin_port=htons(PORT);
 addr.sin_addr.s_addr=INADDR_ANY;
 bind(s,(sockaddr*)&addr,sizeof(addr));
 listen(s,10);
 while(true){
  int c=accept(s,nullptr,nullptr);
  clients.push_back(c);
  std::thread(handle,c).detach();
 }
}
