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
#include "ConcurrentStreamServer.h"

 //int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

void *ConcurrentStreamServer(void *pipefd)
{
  int sockfd_listen=-1, sockfd_connect=-1;
  struct sockaddr_in addr_mine={0};
  fd_set readfds, readfds_temp;
  char buf[HTTP_LEN];
  int length=0;
  int reqfd=0, resfd=0;
  ip_t ip_local = ip[IP_INDEX(TCP,LOCAL)];
  port_t port_local = port[IP_INDEX(TCP,LOCAL)];
  struct timeval timeout={10,0}, timeout_temp={0};
  int ret=0;
  int nPipeReadFlag = 0;
  int pipefd_so[2]={-1};

  reqfd = ((int*)pipefd)[PIPE_INDEX(REQUEST,WRITE)];
  resfd = ((int*)pipefd)[PIPE_INDEX(RESPONSE,READ)];

  ret = pipe(pipefd_so);
  handle_error_ret(ret, "pipe()");

  nPipeReadFlag = fcntl(pipefd_so[0], F_GETFL, 0);
  nPipeReadFlag |= O_NONBLOCK;

  ret = fcntl(pipefd_so[0], F_SETFL, nPipeReadFlag);
  handle_error_ret(ret, "fcntl()");

  addr_mine.sin_family = AF_INET;
  inet_pton(AF_INET,ip_local,&addr_mine.sin_addr);
  addr_mine.sin_port = htons(port_local);

  sockfd_listen = socket(AF_INET, SOCK_STREAM, 0);
  handle_error_ret(sockfd_listen, "socket()");

  ret = bind(sockfd_listen, (struct sockaddr *)&addr_mine, sizeof(struct sockaddr));
  handle_error_ret(ret, "bind()");

  ret = listen(sockfd_listen, 10);
  handle_error_ret(ret, "listen()");

  FD_ZERO(&readfds);

  FD_SET_P(sockfd_listen, &readfds);

  FD_SET_P(resfd, &readfds);

  while(1) {

    timeout_temp = timeout;
    readfds_temp = readfds;

    ret = select(FD_SETSIZE, &readfds_temp, NULL, NULL, NULL);
    handle_error_ret(ret, "ConcurrentStreamServer() select()");

    if(ret == 0) {
      fprintf(stderr,"ERROR:ConcurrentStreamServer() select() timeout\n");
      exit(-1);
    }

    //monitor sockfd_listen
    if(FD_ISSET(sockfd_listen, &readfds_temp)) {


      sockfd_connect = accept(sockfd_listen, (struct sockaddr *)NULL, NULL);
      handle_error_ret(sockfd_connect, "accept()");

      FD_SET_P(sockfd_connect, &readfds);
    }
    //monitor response fd
    else if(FD_ISSET(resfd, &readfds_temp)) {

      length = read(resfd, buf, sizeof(buf)-1);
      handle_error_ret(length, "read()");

      if(length == 0) {
        close(resfd);
        FD_CLR_P(resfd, &readfds);
      }
      else {
        //deal with http packet
        printf("%s(): received from %d:\n%s\n",__FUNCTION__,resfd,buf);

        length = read(pipefd_so[0],&sockfd_connect,sizeof(sockfd_connect));
        handle_error_ret(length, "read() pipefd_so");

        if(length == 0) {
          fprintf(stderr,"pipefd_so closed\n");
          exit(-1);
        }
        else{
          printf("%s(): %d poped out from pipefd_so\n",__FUNCTION__,sockfd_connect);

          if(length == sizeof(errno)) {
            //this http request timed out
          }
          else {
            length = write(sockfd_connect,buf,strlen(buf));
            handle_error_ret(length, "write()");
          }
        }
      }
    }
    //monitor all sockfd_connect
    else {
      for(sockfd_connect=0; sockfd_connect< FD_SETSIZE; sockfd_connect++) {
        if(!FD_ISSET(sockfd_connect, &readfds_temp)) continue; 

        length = read(sockfd_connect, buf, sizeof(buf)-1);
        handle_error_ret(length, "read()");

        if(length == 0) {
          close(sockfd_connect);
          FD_CLR_P(sockfd_connect, &readfds);
        }
        else if(length == (sizeof(buf)-1)) {
          fprintf(stderr,"ERROR: http request is too large to store\n");
          exit(-1);
        }
        else {
          //deal with http packet
          printf("%s(): received from %d:\n%s\n",__FUNCTION__,sockfd_connect,buf);

          length = write(reqfd, buf, sizeof(buf)-1);
          handle_error_ret(length, "write()");

          length = write(pipefd_so[1],&sockfd_connect,sizeof(sockfd_connect));
          handle_error_ret(length, "write()");
        }
      }
    }
  }

}
