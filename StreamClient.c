#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"
#include "StreamClient.h"

int EstablishConnect(ip_t ip, port_t port, int type)
{
  int    sockfd=-1;
  struct sockaddr_in    addr_ser={0};
  struct timeval timeout={60,0};
  int ret=0;

  addr_ser.sin_family = AF_INET;
  inet_pton(AF_INET,ip,&addr_ser.sin_addr);
  addr_ser.sin_port = htons(port);

  sockfd = socket(AF_INET, type, 0);
  handle_error_nn(sockfd, "TRANSMIT","socket()");

  //ret = setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
  //handle_error_nn(ret, "TRANSMIT","setsockopt()");

  //ret = setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
  //handle_error_nn(ret, "TRANSMIT","setsockopt()");

  if (debugl >= 1) {
    log4c_cdn(mycat, info, "TRANSMIT", "Connecting to server[%s:%hd]", ip,port);
  }

  ret = connect(sockfd, (struct sockaddr*)&addr_ser, sizeof(addr_ser));
  handle_error_nn(ret, "TRANSMIT","setsockopt()");

  if (debugl >= 1) {
    log4c_cdn(mycat, info, "TRANSMIT", "Connected");
  }

  return sockfd;
}

void *StreamClient(void *pipefd)
{
  int sockfd_connect=-1;
  fd_set readfds, readfds_temp;
  char buf[HTTP_LEN];
  int length=0;
  int reqfd=0, resfd=0;
  ip_t ip_remote = ip[IP_INDEX(TCP,REMOTE)];
  port_t port_remote = port[IP_INDEX(TCP,REMOTE)];
  struct timeval timeout={10,0}, timeout_temp={0};
  int ret=0;

  //close(((int*)pipefd)[1]);
  //close(((int*)pipefd)[2]);

  reqfd = ((int*)pipefd)[PIPE_INDEX(REQUEST,READ)];
  resfd = ((int*)pipefd)[PIPE_INDEX(RESPONSE,WRITE)];

  sockfd_connect = EstablishConnect(ip_remote, port_remote, SOCK_STREAM);

  FD_ZERO(&readfds);

  FD_SET_P(sockfd_connect, &readfds);

  FD_SET_P(reqfd, &readfds);

  while(1) {

    timeout_temp = timeout;
    readfds_temp = readfds;

    ret = select(FD_SETSIZE, &readfds_temp, NULL, NULL, NULL);
    handle_error_nn(ret, "TRANSMIT","StreamClient() select()");

    if(ret == 0) {
      log4c_cdn(mycat, error, "TRANSMIT", "select() timeout");
      exit(-1);
    }

    //monitor sockfd_connect
    if(FD_ISSET(sockfd_connect, &readfds_temp)) {
      length = read(sockfd_connect, buf, sizeof(buf)-1);
      log4c_cdn(mycat, info, "TRANSMIT", "receiving packet, source=connect, sockfd=%d, length=%d", sockfd_connect, length);
      handle_error_nn(length, "TRANSMIT","read()");

      if(length == 0) {
        close(sockfd_connect);
        FD_CLR_P(sockfd_connect, &readfds);

        sockfd_connect = EstablishConnect(ip_remote, port_remote, SOCK_STREAM);
        FD_SET_P(sockfd_connect, &readfds);
      }
      else if(length == (sizeof(buf)-1)) {
        log4c_cdn(mycat, error, "socket", "http response is too large to store");
        exit(-1);
      }
      else {
        //deal with http packet
        //log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is %s",buf);

        length = write(resfd, buf, sizeof(buf)-1);
        log4c_cdn(mycat, info, "TRANSMIT", "sending packet, destination=resfd, sockfd=%d, length=%d", resfd,length);
        handle_error_nn(length, "TRANSMIT","write()");
      }

      if(!FD_ISSET(reqfd, &readfds)) {
        FD_SET_P(reqfd, &readfds);
      }
    }
    //monitor request fd
    else if(FD_ISSET(reqfd, &readfds_temp)) {
      length = read(reqfd, buf, sizeof(buf)-1);
      log4c_cdn(mycat, info, "TRANSMIT", "receiving packet, source=reqfd, sockfd=%d length=%d", reqfd, length);
      handle_error_nn(length, "TRANSMIT","read()");

      if(length == 0) {
        close(reqfd);
        FD_CLR_P(reqfd, &readfds);
      }
      else {
        //deal with http packet
        //log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is %s",buf);

        length = write(sockfd_connect, buf, strlen(buf));
        log4c_cdn(mycat, info, "TRANSMIT", "sending packet, destination=connect, sockfd=%d, length=%d", sockfd_connect,length);
        handle_error_nn(length,"TRANSMIT","write()");
      }

    }
  }
  return NULL;
}
