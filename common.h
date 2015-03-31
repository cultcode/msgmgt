#ifndef COMMON_H
#define COMMON_H

#define HTTP_LEN 4096
#define BUF_LEN  10

typedef struct HttpBuf_s {
  char buf[HTTP_LEN];
  int  fd;
} HttpBuf_t;

#endif
