#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "common.h"

char * server;
char * url[1];
char * logdir;
char node_3des_key[KEY_LEN]=DEFAULT_NODE_3DES_KEY; 
char node_3des_iv[KEY_LEN]=DEFAULT_NODE_3DES_IV;

ip_t ip[2][2]={{"127.0.0.1","127.0.0.1"},{NULL,NULL}};
port_t port[2][2]={{7070,7070},{0,0}};
char *SelfBaseName;
char *SelfDirName;
int servertimezone=DEFAULT_SERVERTIMEZONE;

long GetLocaltimeSeconds(int servertimezone)
{
  time_t t=time(NULL);

  t += servertimezone * 3600;

  return (long)t;
}

int StripNewLine(char *buf, int length)
{
  int i=0;
  int sum=0;
  //strip the character of line feed / carrier return
  for (i = length-1; i>=0; i--) {
    if((buf[i] == '\n') || (buf[i] == '\r')) {
      sum += 1;
    }   
    else {
      break;
    }   
  }

 return(length-sum);
}

int mkdirs(const char *pathname, mode_t mode) {
  char *p=NULL,tmp[1024]={0};
  int ret=-1;

  if(access(pathname,F_OK) == 0) {
    return 0;
  }

  p = strrchr(pathname, '/');

  if(p) {
    strncpy(tmp, pathname, p-pathname);
    tmp[p-pathname] = '\0';
    ret = mkdirs(tmp,mode);
    if(ret == -1) return ret;
  }

  ret = mkdir(pathname,mode);

  return ret;
}
