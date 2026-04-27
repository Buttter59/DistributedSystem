#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>

void recvMsg(int sock){
 char buf[1024];
 while(true){
  int n=recv(sock,buf,1024,0);
  if(n<=0) break;
  std::cout<<buf;
 }
}

int main(){
 int sock=socket(AF_INET,SOCK_STREAM,0);
 sockaddr_in addr{};
 addr.sin_family=AF_INET;
 addr.sin_port=htons(8080);
 inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
 connect(sock,(sockaddr*)&addr,sizeof(addr));

 std::string name;
 std::getline(std::cin,name);
 send(sock,name.c_str(),name.size(),0);

 std::thread t(recvMsg,sock);
 t.detach();

 while(true){
  std::string msg;
  std::getline(std::cin,msg);
  send(sock,msg.c_str(),msg.size(),0);
 }
}
