#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "common.h"
#include "ConcurrentStreamServer.h"
#include "StreamClient.h"
#include "RoundGramServer.h"
#include "http_parser_package.h"

int debugl = DEFAULT_DEBUGL;

/*********************************************************
 *     local remote 
 * tcp   0     1
 * udp   2     3
 ********************************************************/
ip_t *ip;
port_t *port;

int (*log4c_init)(void); 
void* (*log4c_category_get)(const char* a_name);
void (*log4c_category_userloc_error)( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
void (*log4c_category_userloc_warn )( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
void (*log4c_category_userloc_info )( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
void (*log4c_category_userloc_debug)( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
int (*log4c_fini)(void);
void* mycat;

int main(int argc, char **argv)
{
  void* threadreturn=NULL;
  int pipefd[PIPE_INDEX_MAX]={0};
  int ret=0;
  pthread_t id_StreamClient;
  pthread_t id_ConcurrentStreamServer;
  pthread_t id_RoundGramServer;
  void *handle;  
  char *error;  

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
 * import log4c functions
 ********************************************************/
  handle = dlopen("liblog4c.so", RTLD_LAZY);  
  if (!handle) {  
      fprintf (stderr, "%s\n", dlerror());  
      exit(1);  
  }  

  log4c_init = dlsym(handle, "log4c_init");  
  log4c_category_get = dlsym(handle, "log4c_category_get");  
  log4c_category_userloc_error = dlsym(handle, "log4c_category_userloc_error");  
  log4c_category_userloc_warn  = dlsym(handle, "log4c_category_userloc_warn");  
  log4c_category_userloc_info  = dlsym(handle, "log4c_category_userloc_info");  
  log4c_category_userloc_debug = dlsym(handle, "log4c_category_userloc_debug");  
  log4c_fini = dlsym(handle, "log4c_fini");  

  if ((error = dlerror()) != NULL)  {  
      fprintf (stderr, "%s\n", error);  
      exit(1);  
  }  
  
  /* call log4c functions */
  (*log4c_init)();
  mycat = (*log4c_category_get)("application1");
  
  log4c_cdn(mycat, info, "LOG4C", "log4c loaded");	      

/*********************************************************
 * create pipe
 ********************************************************/
//request pipe
  if (pipe(pipefd+PIPE_INDEX(REQUEST,READ)) == -1) {
    log4c_cdn(mycat, error, "PIPE", "%s",strerror(errno));	      
    exit(EXIT_FAILURE);
  }

//response pipe
  if (pipe(pipefd+PIPE_INDEX(RESPONSE,READ)) == -1) {
    log4c_cdn(mycat, error, "PIPE", "%s",strerror(errno));	      
    exit(EXIT_FAILURE);
  }

/*********************************************************
 * create HTTP parser
 ********************************************************/
  parser_package_init();

/*********************************************************
 * create threads
 ********************************************************/
  if((ret = pthread_create(&id_ConcurrentStreamServer, NULL, &ConcurrentStreamServer, pipefd)) != 0) {
    handle_error_pn(ret, "THREAD", "pthread_create failed");
    exit(ret);
  }

  if((ret = pthread_create(&id_StreamClient, NULL, &StreamClient, pipefd)) != 0) {
    handle_error_pn(ret, "THREAD", "pthread_create failed");
    exit(ret);
  }

  if((ret = pthread_create(&id_RoundGramServer, NULL, &RoundGramServer, 0)) != 0) {
    handle_error_pn(ret, "THREAD", "pthread_create failed");
    exit(ret);
  }

/*********************************************************
 * synchronize
 ********************************************************/
  pthread_join(id_ConcurrentStreamServer, &threadreturn);
  pthread_join(id_StreamClient, &threadreturn);
  pthread_join(id_RoundGramServer, &threadreturn);

/*********************************************************
 * Explicitly call the log4c cleanup routine
 ********************************************************/
  if ( (*log4c_fini)()){
    fprintf(stderr,"log4c_fini() failed");
    exit(1);
  }
  
  dlclose(handle);  

  return 0;
}
