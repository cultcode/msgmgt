#ifndef COMMON_H
#define COMMON_H

extern int debugl;

#define log4c_cdn(a_category, a_priority, a_void, a_format, args...) \
  (*log4c_category_userloc_##a_priority)(a_category, __FILE__, __LINE__, a_void, a_format , ## args );
extern int (*log4c_init)(void);  
extern void* (*log4c_category_get)(const char* a_name);
extern void (*log4c_category_userloc_error)( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
extern void (*log4c_category_userloc_warn )( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
extern void (*log4c_category_userloc_info )( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...);
extern void (*log4c_category_userloc_debug)( const void* a_category, char* file, int   line, void* a_void, const char* a_format, ...); 
extern int (*log4c_fini)(void);
extern void* mycat;

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
  log4c_cdn(mycat, info, "TRANSMIT", "FD_SET %d(%s)", (a), ""#a"");
  

#define FD_CLR_P(a,b) \
  FD_CLR((a),(b));\
  log4c_cdn(mycat, info, "TRANSMIT", "FD_CLR %d(%s)", (a), ""#a"");

#define handle_error(CODE, MSG) \
  do { log4c_cdn(mycat, error, CODE, MSG); exit(EXIT_FAILURE); } while (0);

#define handle_error_pn(ret, CODE, MSG) \
  if(ret > 0) {handle_error(CODE, MSG)}

#define handle_error_nn(ret, CODE, MSG) \
  if(ret < 0) {handle_error(CODE, MSG)}

#endif
