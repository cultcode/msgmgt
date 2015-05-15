#ifndef COMMON_H
#define COMMON_H

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

#define FD_SET_P(a,b) \
  FD_SET((a),(b));\
  log4c_cdn(mycat, debug, "TRANSMIT", "FD_SET %d(%s)", (a), ""#a"");
  

#define FD_CLR_P(a,b) \
  FD_CLR((a),(b));\
  log4c_cdn(mycat, debug, "TRANSMIT", "FD_CLR %d(%s)", (a), ""#a"");

#define handle_error(EXIT, CODE, MSG) \
  do { log4c_cdn(mycat, error, CODE, MSG); if(EXIT) exit(EXIT_FAILURE); } while (0);

#define handle_error_pn(RET, EXIT,  CODE, MSG) \
  if(RET > 0) {handle_error(EXIT, CODE, MSG)}

#define handle_error_nn(RET, EXIT, CODE, MSG) \
  if(RET < 0) {handle_error(EXIT, CODE, MSG)}

#define IP_LEN 32
#define FN_LEN 1024
#define NODENAME_LEN 128
#define VERSION_LEN 32
#define STATUSDESC_LEN 128
#define CONTENT_LEN 1024
#define KEY_LEN 256
#define URL_LEN 256
#define DEFAULT_SERVERTIMEZONE 8
#define DEFAULT_NODE_3DES_KEY  "t^^BvGfAdUTixobQP$HhsOsD"
#define DEFAULT_NODE_3DES_IV   "=V#s%CS)"

#define NODE_3DES_KEY          node_3des_key
#define NODE_3DES_IV           node_3des_iv

extern char * server;
extern char * url[1];
extern ip_t ip[2][2];
extern port_t port[2][2];
extern char *SelfBaseName;
extern char *SelfDirName;
extern int servertimezone;
extern char * logdir;
extern char node_3des_key[KEY_LEN];
extern char node_3des_iv[KEY_LEN];
extern int svrversion;
extern int svrtype;

extern long GetLocaltimeSeconds(int servertimezone);
extern int StripNewLine(char *buf, int length);
extern int mkdirs(const char *pathname, mode_t mode);
#endif
