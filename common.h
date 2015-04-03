#ifndef COMMON_H
#define COMMON_H

extern int debugl;

#define HTTP_LEN 4096
#define BUF_LEN  10

typedef char*   ip_t;
typedef unsigned short port_t;

enum {
  TCP=0,
  UDP=1
};

enum {
  LOCAL=0,
  REMOTE=1
};

#define IP_INDEX(a,b) (((a)<<1)|(b))
#define IP_INDEX_MAX 4

enum {
  REQUEST=0,
  RESPONSE=1
};

enum {
  READ=0,
  WRITE=1
};

#define PIPE_INDEX(a,b) (((a)<<1)|(b))
#define PIPE_INDEX_MAX 4

typedef struct HttpBuf_s {
  char buf[HTTP_LEN];
  int  fd;
} HttpBuf_t;

#define FD_SET_P(a,b) \
  FD_SET((a),(b));\
  if(debugl >= 3) {\
    printf("%s() : FD_SET fd(%s) %d\n", __FUNCTION__, ""#a"", (a));\
  }

#define FD_CLR_P(a,b) \
  FD_CLR((a),(b));\
  if(debugl >= 3) {\
    printf("%s() : FD_CLR fd(%s) %d\n", __FUNCTION__, ""#a"", (a));\
  }

#define handle_error( msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0);

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0);

#define handle_error_ret(ret, msg) \
  if(ret < 0) {handle_error(msg)}

#endif
