#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <curl/curl.h>  

#include "common.h"
#include "OperateXml.h"
#include "InitNodeStatus.h"
#include "ConcurrentStreamServer.h"
#include "StreamClient.h"
#include "RoundGramServer.h"
#include "http_parser_package.h"

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

  char **options=NULL;
  char ConfigXml[FN_LEN] = {0};
  int argcount=0;
  struct stat laststat={0}, tempstat={0};
  int ArgMode=0;
  int i=0;

  char *path0 = NULL, *path1=NULL;;

/********************************************************
 * initialization
 ********************************************************/
  path0 = strdup(argv[0]);
  path1 = strdup(argv[0]);
  SelfBaseName=basename(path0);
  SelfDirName=dirname(path1);

/********************************************************
 * parse xml
 ********************************************************/
  if((argc == 1) || ((argc==2) &&(argv[1][0] != '-'))) {
    if(argc == 1) {
      strcpy(ConfigXml,argv[0]);
      strcat(ConfigXml,".config");
    }else {
      strcpy(ConfigXml,argv[1]);
    }

    if(stat(ConfigXml, &tempstat) != -1) {
      ArgMode = 1;
      goto GET_ARGUMENTS_XML;
    }
    else {
      fprintf(stderr,"ERROR: Neither cmdline arguments nor .xml provieded\n");
      exit(1);
    }
  }
  else {
    ArgMode = 0;
    ret = ParseOptions(argc, argv);

    if(ret) {
      exit(1);
    }
    goto GET_ARGUMENTS_END;
  }

GET_ARGUMENTS_XML:
  memcpy(&laststat,&tempstat, sizeof(struct stat));

  if((argcount = ReadConfigXml(ConfigXml, &options)) <= 0) {
    //exit(1);
  }
  else {
    ret = ParseOptions(argcount, options);

    for(i=0;i<argcount;i++) {
      free(options[i]);
    }
    free(options);
    options = NULL;

    if(ret) {
      //exit(1);
    }
  }

GET_ARGUMENTS_END:
  fflush(stdout);

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
  mycat = (*log4c_category_get)(SelfBaseName);
  
  log4c_cdn(mycat, info, "LOG4C", "log4c loaded");	      

/*********************************************************
 * POST to SvrInit
 ********************************************************/
  if(url[0] && strlen(url[0])) {
    InitNodeStatus();
  }

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
    handle_error_pn(ret, 1, "THREAD", "pthread_create failed");
    exit(ret);
  }

  if((ret = pthread_create(&id_StreamClient, NULL, &StreamClient, pipefd)) != 0) {
    handle_error_pn(ret, 1, "THREAD", "pthread_create failed");
    exit(ret);
  }

  if((ret = pthread_create(&id_RoundGramServer, NULL, &RoundGramServer, 0)) != 0) {
    handle_error_pn(ret, 1,  "THREAD", "pthread_create failed");
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
