#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "xevent.h"
#include "winsock2.h"
#define sleep(sec) Sleep(sec*1000)
#pragma comment(lib, "ws2_32.lib")
#include <sys/types.h>
#include <stdio.h>

// service of server
int callback_service(int fd, int filter){
  LOG_I("serv-fd=%d, filter=%d(%s)", fd, filter, xfilterdesc(filter));
  if (filter == xfilter_read) {
    char buff[256] = { 0 };
    if (recv(fd, buff, sizeof(buff) - 1, 0) > 0) {
      LOG_I("Server Recv: %s", buff);
    }
    char reply[] = "hi from server";
    if (send(fd, reply, sizeof(reply), 0) == sizeof(reply)){
      LOG_I("Server Send: %s", reply);
    }
  }
  else if (filter == xfilter_error){
    LOG_E("receive fd error, filter %d", filter);
	unregxevent(fd);
  }
  return 0;
}

// accept new connect and add reg connect event proc
int callback_accept(int srvfd, int filter) {
  if (filter == xfilter_read){
    int clifd = -1;
    struct sockaddr_in cli;
    unsigned int clilen = sizeof(cli);
    if ((clifd = accept(srvfd, (sockaddr*)&cli, &clilen)) < 0){
        LOG_E("accept error[%d], ignored!", errno); 
        return -1;
    }
    regxevent(clifd, xfilter_read, callback_service);
  }
  else if (filter == xfilter_error){
    LOG_E("receive srvfd error, filter %d", filter);
    unregxevent(srvfd);
  }
  return 0;
}

// client send message to server and receive reply
int callback_client(int fd, int filter){
  LOG_I("client-fd=%d, filter=%d(%s)", fd, filter, xfilterdesc(filter));
  if (filter == xfilter_read) {
    char buff[256] = { 0 };
    if (recv(fd, buff, sizeof(buff) - 1, 0) > 0) {
      LOG_I("Client Recv: %s", buff);
    }
    char reply[] = "hi from client";
    if (send(fd, reply, sizeof(reply), 0) == sizeof(reply)){
      LOG_I("Client Send: %s", reply);
    }

  }
  else if (filter == xfilter_write) {
    char buff[] = "first hi message from client";
    if (send(fd, buff, sizeof(buff), 0) == sizeof(buff)) {
      LOG_I("Client Send: %s", buff);
    }
    unregxevent(fd, xfilter_write);
  }
  else{
    LOG_E("receive fd error, filter %d", filter);
    unregxevent(fd);
  }
  sleep(2);
  return 0;
}
int main(int argc, char** argv)
{
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);    // init winsock
  if (iResult != 0) {
    LOG_E("WSAStartup failed: %d\n", iResult);
    return 1;
  }

  // server and client socket
  struct sockaddr_in serv = { 0 }, cli = {0};
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = htonl(INADDR_ANY);
  serv.sin_port = htons(18080);
  cli.sin_family= AF_INET;
  cli.sin_addr.s_addr = inet_addr("127.0.0.1");
  cli.sin_port = htons(18080);

  // start server listen
  int fd = -1, clifd = -1;
  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    LOG_E("init socket failed!");
    return -1;
  }
  if (bind(fd, (struct sockaddr*)&serv, sizeof(serv)) == -1) {
    LOG_E("bind server failed!");
    return -1;
  }
  if (listen(fd, 511) == -1) {
    LOG_E("listen failed!");
    return -1;
  }
  printf("listen port 18080\n");

  // register server proc
  initxevent();
  regxevent(fd, xfilter_read, callback_accept);

  // start client connect to server
  clifd = socket(PF_INET, SOCK_STREAM, 0);
  if (connect(clifd, (sockaddr*)(&cli), sizeof(cli)) == -1){
    LOG_E("connect failed!");
    return -1;
  }

  // register client proc
  regxevent(clifd, xfilter_read, callback_client);
  regxevent(clifd, xfilter_write, callback_client);
  regxevent(clifd, xfilter_error, callback_client);

  // dispatch message
  while (1) {
    dispatchxevent(3);
    if (xeventnum() == 0) {
      LOG_R("finish dispath");
      break;
    }
  }

  WSACleanup(); // cleanup winsock
  
  return 0;
}
