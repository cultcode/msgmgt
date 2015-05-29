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
  char buf[HTTP_LEN]={0};
  int length=0;
  int ret=0;
  struct timeval timeout={10,0};
  socklen_t addrlen;
  ip_t ip_local  = ip[LOCAL][UDP];
  ip_t ip_remote = ip[REMOTE][UDP];
  port_t port_local =  port[LOCAL][UDP];
  port_t port_remote = port[REMOTE][UDP];

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
  handle_error_nn(sockfd_me4cli, 1, "TRANSMIT", "socket() %s",strerror(errno));

  ret = bind(sockfd_me4cli, (struct sockaddr *)&addr_me4cli, addrlen);
  handle_error_nn(ret, 1, "TRANSMIT", "bind() %s",strerror(errno));

/*********************************************************
 * sockfd_me4ser
 ********************************************************/
  sockfd_me4ser = socket(AF_INET, SOCK_DGRAM, 0);
  handle_error_nn(sockfd_me4ser, 1, "TRANSMIT", "socket() %s",strerror(errno));

  ret = setsockopt(sockfd_me4ser,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
  handle_error_nn(ret, 1, "TRANSMIT", "setsockopt() %s",strerror(errno));

  ret = setsockopt(sockfd_me4ser,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
  handle_error_nn(ret, 1, "TRANSMIT", "setsockopt() %s",strerror(errno));

/*********************************************************
 * while
 ********************************************************/
  while(1) {
    //cli -> me -> ser
    memset(buf,0,sizeof(buf));
    length = recvfrom(sockfd_me4cli, buf, sizeof(buf)-1, 0, (struct sockaddr *)&addr_cli, &addrlen);
    log4c_cdn(mycat, info, "TRANSMIT", "receiving <== me2cli=%d length=%d", sockfd_me4cli, length);
    handle_error_nn(length, 1, "TRANSMIT", "recvfrom() %s",strerror(errno));

    if(length == (sizeof(buf)-1)) {
      log4c_cdn(mycat, error, "TRANSMIT", "http request is too large to store");
      exit(-1);
    }
    else {
      //deal with http packet
      log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);

      length = sendto(sockfd_me4ser, buf, strlen(buf), 0, (struct sockaddr *)&addr_ser, addrlen);
      log4c_cdn(mycat, info, "TRANSMIT", "sending ==> me2ser=%d, length=%d", sockfd_me4ser,length);
      handle_error_nn(length, 1, "TRANSMIT", "sendto %s",strerror(errno));
    }

//    //ser -> me -> cli
//    length = recvfrom(sockfd_me4ser, buf, sizeof(buf)-1, 0, (struct sockaddr *)&addr_ser, &addrlen);
//    if(length == -1) {
//      if(errno == EAGAIN) {
//        sprintf(buf,"%d",errno);
//      }
//      else {
//        handle_error("TRANSMIT", 1, "recvfrom() sockfd_me4ser");
//      }
//    }
//    else if(length == (sizeof(buf)-1)) {
//      fprintf(stderr,"ERROR: http request is too large to store\n");
//      exit(-1);
//    }
//    else {
//      //deal with http packet
//      printf("%s(): received from %d:\n%s\n",__FUNCTION__, sockfd_me4ser,buf);
//
//      length = sendto(sockfd_me4cli, buf, strlen(buf), 0, (struct sockaddr *)&addr_cli, addrlen);
//      handle_error_nn(length, 1, "TRANSMIT", "sendto %s",strerror(errno));
//    }
  }

}
