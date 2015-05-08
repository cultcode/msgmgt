#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"
#include "RoundGramServer.h"

#define EQUAL_SOCKADDF_IN(A,B) (A).sin_family==(B).sin_family && (A).sin_port==(B).sin_port && (A).sin_addr.s_addr==(B).sin_addr.s_addr

void *RoundGramServer(void *data)
{
  int sockfd_me4cli=-1, sockfd_me4ser=-1;
  struct sockaddr_in addr_me4cli={0},addr_cli={0},addr_ser={0};
  char buf[HTTP_LEN];
  int length=0;
  int ret=0;
  struct timeval timeout={10,0};
  socklen_t addrlen;
  ip_t ip_local  = ip[IP_INDEX(UDP,LOCAL)];
  ip_t ip_remote = ip[IP_INDEX(UDP,REMOTE)];
  port_t port_local = port[IP_INDEX(UDP,LOCAL)];
  port_t port_remote = port[IP_INDEX(UDP,REMOTE)];

  addrlen = sizeof(struct sockaddr_in);

  addr_me4cli.sin_family = AF_INET;
  inet_pton(AF_INET,ip_local,&addr_me4cli.sin_addr);
  addr_me4cli.sin_port = htons(port_local);

  addr_ser.sin_family = AF_INET;
  inet_pton(AF_INET,ip_remote,&addr_ser.sin_addr);
  addr_ser.sin_port = htons(port_remote);

/*********************************************************
 * sockfd_me4cli
 ********************************************************/
  sockfd_me4cli = socket(AF_INET, SOCK_DGRAM, 0);
  handle_error_nn(sockfd_me4cli, "TRANSMIT", "socket()");

  ret = bind(sockfd_me4cli, (struct sockaddr *)&addr_me4cli, addrlen);
  handle_error_nn(ret, "TRANSMIT", "bind()");

/*********************************************************
 * sockfd_me4ser
 ********************************************************/
  sockfd_me4ser = socket(AF_INET, SOCK_DGRAM, 0);
  handle_error_nn(sockfd_me4ser, "TRANSMIT", "socket()");

  ret = setsockopt(sockfd_me4ser,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
  handle_error_nn(ret, "TRANSMIT", "setsockopt()");

  ret = setsockopt(sockfd_me4ser,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
  handle_error_nn(ret, "TRANSMIT", "setsockopt()");

/*********************************************************
 * while
 ********************************************************/
  while(1) {
    //cli -> me -> ser
    length = recvfrom(sockfd_me4cli, buf, sizeof(buf)-1, 0, (struct sockaddr *)&addr_cli, &addrlen);
    handle_error_nn(length, "TRANSMIT", "recvfrom()");

    if(length == (sizeof(buf)-1)) {
      fprintf(stderr,"ERROR: http request is too large to store\n");
      exit(-1);
    }
    else {
      //deal with http packet
      printf("%s(): received from %d:\n%s\n",__FUNCTION__, sockfd_me4cli,buf);

      printf("%s(): sent     to   %d:\n%s\n",__FUNCTION__, sockfd_me4ser,buf);

      length = sendto(sockfd_me4ser, buf, strlen(buf), 0, (struct sockaddr *)&addr_ser, addrlen);
      handle_error_nn(length, "TRANSMIT", "sendto");
    }

    //ser -> me -> cli
    length = recvfrom(sockfd_me4ser, buf, sizeof(buf)-1, 0, (struct sockaddr *)&addr_ser, &addrlen);
    if(length == -1) {
      if(errno == EAGAIN) {
        sprintf(buf,"%d",errno);
      }
      else {
        handle_error("TRANSMIT", "recvfrom() sockfd_me4ser");
      }
    }
    else if(length == (sizeof(buf)-1)) {
      fprintf(stderr,"ERROR: http request is too large to store\n");
      exit(-1);
    }
    else {
      //deal with http packet
      printf("%s(): received from %d:\n%s\n",__FUNCTION__, sockfd_me4ser,buf);

      length = sendto(sockfd_me4cli, buf, strlen(buf), 0, (struct sockaddr *)&addr_cli, addrlen);
      handle_error_nn(length, "TRANSMIT", "sendto");
    }
  }

}
