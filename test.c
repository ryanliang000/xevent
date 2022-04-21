#if defined(_WIN32) || defined(_WIN64)
  #define _WINSOCK_DEPRECATED_NO_WARNINGS
  #define _CRT_SECURE_NO_WARNINGS
  #include "xevent.h"
  #include "winsock2.h"
  #define sleep(sec) Sleep(sec*1000)
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include "xevent.h"
#endif
#include <sys/types.h>
#include <stdio.h>


int callback(int fd, int filter)
{
  LOG_I("fd=%d, filter=%d(%s)", fd, filter, xfilterdesc(filter));
  if (filter == xfilter_read) {
    char buff[256] = { 0 };
    if (recv(fd, buff, sizeof(buff) - 1, 0) > 0) {
      LOG_I("Recv: %s", buff);
    }
    else {
      LOG_R("recv fail, close connect.");
      unregxevent(fd);
    }
  }
  else if (filter == xfilter_write) {
    char buff[] = "GET / \n\n";
    if (send(fd, buff, sizeof(buff), 0) == sizeof(buff)) {
      LOG_I("Send: %s", buff);
    }
    else {
      LOG_E("send error");
      unregxevent(fd);
    }
  }
  return 0;
}

int main(int argc, char** argv)
{
#if defined(_WIN32) || defined(_WIN64)
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);    // init winsock
  if (iResult != 0) {
    LOG_E("WSAStartup failed: %d\n", iResult);
    return 1;
  }
#endif

  struct sockaddr_in serv = { 0 };
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = inet_addr("182.61.200.6");
  serv.sin_port = htons(80);
  int fd = -1;
  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    LOG_E("init socket failed!");
    return -1;
  }
  if (connect(fd, (sockaddr*)(&serv), sizeof(serv)) < 0) {
    LOG_E("connect server failed!");
    return -1;
  }

  initxevent();
  regxevent(fd, xfilter_read, callback);
  regxevent(fd, xfilter_write, callback);
  regxevent(fd, xfilter_error, callback);
  while (1) {
    dispatchxevent(3);
    sleep(1);
    if (xeventnum() == 0) {
      LOG_R("finish dispath");
      break;
    }
  }
  
  
#if defined(_WIN32) || defined(_WIN64)
  WSACleanup(); // cleanup winsock
#endif

  return 0;
}
