#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "common.h"
#include "ConcurrentStreamServer.h"

__attribute__((weak)) int debugl = DEFAULT_DEBUGL;

 //int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

void *ConcurrentStreamServer(void *pipefd)
{
  int sockfd_listen=-1, sockfd_connect=-1;
  struct sockaddr_in myaddr={0};
  int i=0;
  fd_set readfds;
  char buf[HTTP_LEN];
  int length=0;
  int reqfd=0, resfd=0;
  typeof(ip_local) ip = ip_local;
  typeof(port_local) port = port_local;

  close(((int*)pipefd)[0]);
  close(((int*)pipefd)[3]);

  reqfd = ((int*)pipefd)[1];
  resfd = ((int*)pipefd)[2];

  myaddr.sin_family = AF_INET;
  myaddr.sin_port = htons(port);

  if( inet_pton(AF_INET, ip, &myaddr.sin_addr) <= 0){
    fprintf(stderr,"ERROR: inet_pton() failed when convert ip:[%s]\n",ip);
    exit(1);
  }

  if((sockfd_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() The following error occurred");
    exit(1);
  }

  if(bind(sockfd_listen, (struct sockaddr *)&myaddr, sizeof(struct sockaddr)) < 0) {
    perror("bind() The following error occurred");
    exit(1);
  }

  if(listen(sockfd_listen, 10) < 0) {
    perror("listen() The following error occurred");
    exit(1);
  }

  FD_ZERO(&readfds);

  FD_SET(sockfd_listen, &readfds);
  printf("FD_SET sockfd_listen %d\n", sockfd_listen);

  FD_SET(resfd, &readfds);
  printf("FD_SET response fd %d\n", resfd);

  while(1) {

    select(FD_SETSIZE,&readfds, NULL, NULL, NULL);

    //monitor sockfd_listen
    if(FD_ISSET(sockfd_listen, &readfds)) {


      if((sockfd_connect = accept(sockfd_listen, (struct sockaddr *)NULL, NULL)) < 0) {
        perror("accept() The following error occurred");
        exit(1);
      }

      FD_SET(sockfd_connect, &readfds);
      printf("FD_SET sockfd_connect %d\n", sockfd_connect);
    }
    //monitor response fd
    else if(FD_ISSET(resfd, &readfds)) {
      i = resfd;

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
        //write();
      }
    }
    //monitor all sockfd_connect
    else {
      for(i=0; i< FD_SETSIZE; i++) {
        if(!FD_ISSET(i, &readfds)) continue; 

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
          write(reqfd, buf, sizeof(buf)-1);
        }
      }
    }
  }

}
