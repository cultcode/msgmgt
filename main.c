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
#include "RoundGramServer.h"

int debugl = DEFAULT_DEBUGL;
ip_t *ip;
port_t *port;
/*********************************************************
 *     local remote 
 * tcp   0     1
 * udp   2     3
 ********************************************************/

int main(int argc, char **argv)
{
  void* threadreturn=NULL;
  int pipefd[PIPE_INDEX_MAX]={0};
  int ret=0;
  pthread_t id_StreamClient;
  pthread_t id_ConcurrentStreamServer;
  pthread_t id_RoundGramServer;

/*********************************************************
 * parse config
 ********************************************************/
  assert(argc == 5);

  ip = calloc(sizeof(ip_t),IP_INDEX_MAX);
  port = calloc(sizeof(port_t),IP_INDEX_MAX);

  ip[IP_INDEX(TCP,LOCAL)] = argv[1];
  port[IP_INDEX(TCP,LOCAL)] = atoi(argv[2]);

  ip[IP_INDEX(UDP,LOCAL)] = ip[IP_INDEX(TCP,LOCAL)];
  port[IP_INDEX(UDP,LOCAL)] = port[IP_INDEX(TCP,LOCAL)];

  ip[IP_INDEX(TCP,REMOTE)] = argv[3];
  port[IP_INDEX(TCP,REMOTE)] = atoi(argv[4]);

  ip[IP_INDEX(UDP,REMOTE)] = ip[IP_INDEX(TCP,REMOTE)];
  port[IP_INDEX(UDP,REMOTE)] = port[IP_INDEX(TCP,REMOTE)];

/*********************************************************
 * create pipe
 ********************************************************/
//request pipe
  if (pipe(pipefd+PIPE_INDEX(REQUEST,READ)) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

//response pipe
  if (pipe(pipefd+PIPE_INDEX(RESPONSE,READ)) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

/*********************************************************
 * create threads
 ********************************************************/
  if((ret = pthread_create(&id_ConcurrentStreamServer, NULL, &ConcurrentStreamServer, pipefd)) != 0) {
    handle_error_en(ret, "pthread_create");
    exit(ret);
  }

  if((ret = pthread_create(&id_StreamClient, NULL, &StreamClient, pipefd)) != 0) {
    handle_error_en(ret, "pthread_create");
    exit(ret);
  }

  if((ret = pthread_create(&id_RoundGramServer, NULL, &RoundGramServer, 0)) != 0) {
    handle_error_en(ret, "pthread_create");
    exit(ret);
  }

/*********************************************************
 * synchronize
 ********************************************************/
  pthread_join(id_ConcurrentStreamServer, &threadreturn);
  pthread_join(id_StreamClient, &threadreturn);
  pthread_join(id_RoundGramServer, &threadreturn);

  return 0;
}
