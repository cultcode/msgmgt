#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "common.h"
#include "StreamClient.h"

void *StreamClient(void *pipefd)
{
  int sockfd_connect=-1;
  struct sockaddr_in seraddr={0};
  int i=0;
  fd_set readfds;
  char buf[HTTP_LEN];
  int length=0;
  int reqfd=0, resfd=0;
  typeof(ip_remote) ip = ip_remote;
  typeof(port_remote) port = port_remote;

  close(((int*)pipefd)[1]);
  close(((int*)pipefd)[2]);

  reqfd = ((int*)pipefd)[0];
  resfd = ((int*)pipefd)[3];

  seraddr.sin_family = AF_INET;
  seraddr.sin_port = htons(port);

  if( inet_pton(AF_INET, ip, &seraddr.sin_addr) <= 0){
    fprintf(stderr,"ERROR: inet_pton() failed when convert ip:[%s]\n",ip);
    exit(1);
  }

  if((sockfd_connect = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() The following error occurred");
    exit(1);
  }

  if(connect(sockfd_connect, (struct sockaddr *)&seraddr, sizeof(struct sockaddr)) < 0) {
    perror("connect() The following error occurred");
    exit(1);
  }

  FD_ZERO(&readfds);

  FD_SET(sockfd_connect, &readfds);
  printf("FD_SET sockfd_connect %d\n", sockfd_connect);

  FD_SET(reqfd, &readfds);
  printf("FD_SET reqponse fd %d\n", reqfd);

  while(1) {

    select(FD_SETSIZE,&readfds, NULL, NULL, NULL);

    //monitor sockfd_listen
    if(FD_ISSET(sockfd_connect, &readfds)) {
      i = sockfd_connect;

      length = read(i, buf, sizeof(buf)-1);
      if(length == -1) {
        perror("read()");
        exit(-1);
      }
      else if(length == 0) {
        close(i);
        FD_CLR(i, &readfds);
        printf("FD_CLR fd %d\n", i);
      }
      else {
        //deal with http packet
        printf("received from %d:%s\n",i,buf);
        write(resfd, buf, sizeof(buf)-1);
      }
    }
    //monitor request fd
    else if(FD_ISSET(reqfd, &readfds)) {
      i = reqfd;

      length = read(i, buf, sizeof(buf)-1);
      if(length == -1) {
        perror("read()");
        exit(-1);
      }
      else if(length == 0) {
        close(i);
        FD_CLR(i, &readfds);
        printf("FD_CLR fd %d\n", i);
      }
      else {
        //deal with http packet
        printf("received from %d:%s\n",i,buf);
        write(sockfd_connect, buf, sizeof(buf)-1);
      }
    }
  }
  return NULL;
}
