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
//  struct timeval timeout={60,0};
  struct linger so_linger={1, 30};
  int flag=1;
  int ret=0;

  addr_ser.sin_family = AF_INET;
  inet_pton(AF_INET,ip,&addr_ser.sin_addr);
  addr_ser.sin_port = htons(port);

  sockfd = socket(AF_INET, type, 0);
  handle_error_nn(sockfd, 1, "TRANSMIT","socket() %s",strerror(errno));

//  ret = setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
//  handle_error_nn(ret, 1,  "TRANSMIT","setsockopt() %s",strerror(errno));
//
//  ret = setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
//  handle_error_nn(ret, 1, "TRANSMIT","setsockopt() %s",strerror(errno));

  ret = setsockopt(sockfd,SOL_SOCKET,SO_LINGER, &so_linger, sizeof(so_linger));
  handle_error_nn(ret, 1, "TRANSMIT","setsockopt() %s",strerror(errno));

  ret = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
  handle_error_nn(ret, 1, "TRANSMIT","setsockopt() %s",strerror(errno));

  log4c_cdn(mycat, info, "TRANSMIT", "Connect to server[%s:%hd]", ip,port);

  ret = connect(sockfd, (struct sockaddr*)&addr_ser, sizeof(addr_ser));
  handle_error_nn(ret, 1, "TRANSMIT","connect() %s",strerror(errno));

  return sockfd;
}

void *StreamClient(void *pipefd)
{
  int sockfd_connect=-1;
  fd_set readfds, readfds_temp;
  char buf[HTTP_LEN]={0};
  char buf1[HTTP_LEN]={0};
  char str1[PORT_LEN]={0}, str2[PORT_LEN]={0};
  int length=0;
  int reqfd=0, resfd=0;
  ip_t ip_remote = ip[REMOTE][TCP];
  port_t port_remote = port[REMOTE][TCP];
  struct timeval timeout={10,0}, timeout_temp={0};
  int ret=0;

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
    handle_error_nn(ret, 1, "TRANSMIT","select() %s",strerror(errno));

    if(ret == 0) {
      log4c_cdn(mycat, error, "TRANSMIT", "select() timeout");
      exit(-1);
    }

    //monitor sockfd_connect
    if(FD_ISSET(sockfd_connect, &readfds_temp)) {
      memset(buf,0,sizeof(buf));
      length = read(sockfd_connect, buf, sizeof(buf)-1);
      handle_error_nn(length, 0, "TRANSMIT","read() %s",strerror(errno));
      log4c_cdn(mycat, info, "TRANSMIT", "receiving packet, source=connect, sockfd=%d, length=%d", sockfd_connect, length);
      log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);

      if(length <= 0) {
        ret = sd_close(sockfd_connect);
        handle_error_nn(ret, 1, "TRANSMIT","close() %s",strerror(errno));
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
        length = write(resfd, buf, strlen(buf));
        handle_error_nn(length, 1, "TRANSMIT","write() %s",strerror(errno));
        log4c_cdn(mycat, info, "TRANSMIT", "sending packet, destination=resfd, sockfd=%d, length=%d", resfd,length);
      }

    }
    //monitor request fd
    /*else*/ if(FD_ISSET(reqfd, &readfds_temp)) {
      memset(buf,0,sizeof(buf));
      length = read(reqfd, buf, sizeof(buf)-1);
      handle_error_nn(length, 1, "TRANSMIT","read() %s",strerror(errno));
      log4c_cdn(mycat, info, "TRANSMIT", "receiving packet, source=reqfd, sockfd=%d length=%d", reqfd, length);

      if(length == 0) {
        ret = sd_close(reqfd);
        handle_error_nn(ret, 1, "TRANSMIT","close() %s",strerror(errno));
        FD_CLR_P(reqfd, &readfds);
      }
      else {
        strcpy(buf1, buf);
        strrpl(buf, buf1, "close", "keep-alive");
        strcpy(buf1, buf);
        strrpl(buf, buf1, "Close", "keep-alive");
        strcpy(buf1, buf);
        strrpl(buf, buf1, ip[LOCAL][TCP], server);
        strcpy(buf1, buf);
        sprintf(str1,"%hd",port[LOCAL][TCP]);
        sprintf(str2,"%hd",port[REMOTE][TCP]);
        strrpl(buf, buf1, str1, str2);

        length = write(sockfd_connect, buf, strlen(buf));
        handle_error_nn(length, 1, "TRANSMIT","write() %s",strerror(errno));
        log4c_cdn(mycat, info, "TRANSMIT", "sending packet, destination=connect, sockfd=%d, length=%d", sockfd_connect,length);
        log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);
      }

    }
  }
  return NULL;
}
