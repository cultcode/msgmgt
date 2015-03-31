#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "ConcurrentStreamServer.h"
#include "StreamClient.h"

int debugl = DEFAULT_DEBUGL;
char *ip_remote;
short port_remote;
char *ip_local;
short port_local;

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char **argv)
{
  void* threadreturn=NULL;
  int pipefd[4]={0};
  int ret=0;
  pthread_t id_StreamClient;
  pthread_t id_ConcurrentStreamServer;

/*********************************************************
 * parse config
 ********************************************************/
  assert(argc == 5);
  ip_local = argv[1];
  port_local = atoi(argv[2]);
  ip_remote = argv[3];
  port_remote = atoi(argv[4]);

/*********************************************************
 * create pipe
 ********************************************************/
//request pipe
  if (pipe(pipefd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

//response pipe
  if (pipe(pipefd+2) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  if((ret = pthread_create(&id_ConcurrentStreamServer, NULL, &ConcurrentStreamServer, pipefd)) != 0) {
    handle_error_en(ret, "pthread_create");
    exit(ret);
  }

  if((ret = pthread_create(&id_StreamClient, NULL, &StreamClient, pipefd)) != 0) {
    handle_error_en(ret, "pthread_create");
    exit(ret);
  }

  pthread_join(id_ConcurrentStreamServer, &threadreturn);
  pthread_join(id_StreamClient, &threadreturn);

  return 0;
}
