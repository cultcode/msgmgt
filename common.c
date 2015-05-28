#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/errno.h>

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

int sd_close(int sockfd)
{
  int ret=0;
CLOSE_SOCKFD:
  ret = close(sockfd);
  if((ret == -1) && (errno == EINTR)) goto CLOSE_SOCKFD;

  return ret;
}

int sd_read(int sockfd, char* buf, int len)
{
  int ret=0;
READ_SOCKFD:
  ret = read(sockfd, buf, len);
  if((ret == -1) && (errno == EINTR)) goto READ_SOCKFD;
  return ret;
}

int sd_write(int sockfd, char* buf, int len)
{
  int ret=0;
WRITE_SOCKFD:
  ret = write(sockfd, buf, len);
  if((ret == -1) && (errno == EINTR)) goto WRITE_SOCKFD;
  return ret;
}

int sd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int ret=0;
ACCEPT_SOCKFD:
  ret = accept(sockfd, addr, addrlen);
  if((ret == -1) && (errno == EINTR)) goto ACCEPT_SOCKFD;
  return ret;
}

void strrpl(char* pDstOut, char* pSrcIn, const char* pSrcRpl, const char* pDstRpl)
{ 
char* pi = pSrcIn; 
char* po = pDstOut; 

int nSrcRplLen = strlen(pSrcRpl); 
int nDstRplLen = strlen(pDstRpl); 

char *p = NULL; 
int nLen = 0; 

do 
{
// 找到下一个替换点
p = strstr(pi, pSrcRpl); 

if(p != NULL) 
{ 
// 拷贝上一个替换点和下一个替换点中间的字符串
nLen = p - pi; 
memcpy(po, pi, nLen);

// 拷贝需要替换的字符串
memcpy(po + nLen, pDstRpl, nDstRplLen); 
} 
else 
{ 
strcpy(po, pi); 

// 如果没有需要拷贝的字符串,说明循环应该结束
break;
} 

pi = p + nSrcRplLen; 
po = po + nLen + nDstRplLen; 

} while (p != NULL); 
}
