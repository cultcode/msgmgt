#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <libgen.h>
#include <sys/queue.h>
#include "common.h"
#include "ConcurrentStreamServer.h"
#include "http_parser.h"
#include "http_parser_package.h"
#include "uthash.h"

#define HTTP_LEN 4096
#define HASH_KEY_LEN 256
#define INTERFACE_LEN 256

struct my_struct {
    char name[HASH_KEY_LEN];             /* key (string is WITHIN the structure) */
    int id;
    int sent;
    UT_hash_handle hh;         /* makes this structure hashable */
};
static struct my_struct *users=NULL;

DEFINE_TAILQ(svrinit)
DEFINE_TAILQ(GetNodeSvrSysParmList)
DEFINE_TAILQ(ReportTaskStatus)
DEFINE_TAILQ(other)

 //int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
static struct message message_response;
static struct message message_request;

void *ConcurrentStreamServer(void *pipefd)
{
  int sockfd_listen=-1, sockfd_connect=-1;
  struct sockaddr_in addr_mine={0};
  fd_set readfds, readfds_temp;
  char buf[HTTP_LEN]={0};
  int length=0;
  int reqfd=0, resfd=0;
  ip_t ip_local = ip[LOCAL][TCP];
  port_t port_local = port[LOCAL][TCP];
  struct timeval timeout={10,0}, timeout_temp={0};
  int ret=0, ret_parse=0;
  //int nPipeReadFlag = 0;
  //int pipefd_so[2]={-1};
  int i=0;
  struct my_struct *s=NULL, *tmp=NULL;
  char *key=NULL;
  char *url=NULL, *cookie=NULL;
  char *token=NULL;
  struct tailq_entry_svrinit *item_svrinit;
  struct tailq_entry_GetNodeSvrSysParmList *item_GetNodeSvrSysParmList;
  struct tailq_entry_ReportTaskStatus *item_ReportTaskStatus;
  struct tailq_entry_other *item_other;

  TAILQ_INIT(&tailq_svrinit_head);
  TAILQ_INIT(&tailq_GetNodeSvrSysParmList_head);
  TAILQ_INIT(&tailq_ReportTaskStatus_head);
  TAILQ_INIT(&tailq_other_head);

  reqfd = ((int*)pipefd)[PIPE_INDEX(REQUEST,WRITE)];
  resfd = ((int*)pipefd)[PIPE_INDEX(RESPONSE,READ)];

  addr_mine.sin_family = AF_INET;
  inet_pton(AF_INET,ip_local,&addr_mine.sin_addr);
  addr_mine.sin_port = htons(port_local);

  sockfd_listen = socket(AF_INET, SOCK_STREAM, 0);
  handle_error_nn(sockfd_listen, 1, "TRANSMIT", "socket() %s",strerror(errno));

  ret = bind(sockfd_listen, (struct sockaddr *)&addr_mine, sizeof(struct sockaddr));
  handle_error_nn(ret, 1, "TRANSMIT", "bind() to %s:%hd %s",ip_local,port_local, strerror(errno));

  ret = listen(sockfd_listen, 10);
  handle_error_nn(ret, 1, "TRANSMIT", "listen() %s",strerror(errno));

  FD_ZERO(&readfds);

  FD_SET_P(sockfd_listen, &readfds);

  FD_SET_P(resfd, &readfds);

  while(1) {

    timeout_temp = timeout;
    readfds_temp = readfds;

    ret = select(FD_SETSIZE, &readfds_temp, NULL, NULL, NULL);
    handle_error_nn(ret, 1, "TRANSMIT", "select() %s",strerror(errno));

    if(ret == 0) {
      log4c_cdn(mycat, error, "TRANSMIT", "select() timeout");
      exit(-1);
    }

    //monitor sockfd_listen
    if(FD_ISSET(sockfd_listen, &readfds_temp)) {
      sockfd_connect = accept(sockfd_listen, (struct sockaddr *)NULL, NULL);
      handle_error_nn(sockfd_connect, 1, "TRANSMIT", "accept() %s",strerror(errno));

      FD_SET_P(sockfd_connect, &readfds);
    }
    //monitor response fd
    /*else*/ if(FD_ISSET(resfd, &readfds_temp)) {

      memset(&message_response, 0, sizeof(message_response));
      memset(buf,0,sizeof(buf));

      length = read(resfd, buf, sizeof(buf)-1);
      handle_error_nn(length, 1, "TRANSMIT", "read() %s",strerror(errno));
      log4c_cdn(mycat, info, "TRANSMIT", "receiving <== resfd=%d length=%d", resfd, length);

      if(length == 0) {
        ret = sd_close(resfd);
        handle_error_nn(ret, 1, "TRANSMIT","close() %s",strerror(errno));
        FD_CLR_P(resfd, &readfds);
      }
      else {
        //deal with http packet

        message_response.raw = buf;
        if((ret_parse=parse_messages(HTTP_RESPONSE, 1, &message_response)) != 0) {
          log4c_cdn(mycat, warn, "TRANSMIT", "Packet unknown and discarded %d, content is\n%s",ret_parse,buf);
        }
        else {
          for(i=0; i<message_response.num_headers; i++) {
            if(!strcasecmp(message_response.headers[i][0], "Set-Cookie" )) {

              cookie = calloc(1, strlen(message_response.headers[i][1])+1);
              strcpy(cookie,message_response.headers[i][1]);
              key = strstr(cookie,"Interface=");
              if(key == NULL) {i = message_response.num_headers;break;}
              key += strlen("Interface=");
              token = strchr(key, ' '); if(token) *token = '\0';
              token = strchr(key, ';'); if(token) *token = '\0';

              HASH_FIND_STR( users, key, s);

              if(s == NULL) {
                log4c_cdn(mycat, error, "HASH", "%s=>NULL found from hash", key);
                exit(1);
              }
              else {
                sockfd_connect = s->id;
                log4c_cdn(mycat, debug, "HASH", "%s=>%d found from hash",key, s->id);
              }

              REMOVE_TAILQ(svrinit);
              REMOVE_TAILQ(GetNodeSvrSysParmList);
              REMOVE_TAILQ(ReportTaskStatus);

              free(cookie);

              break;
            }
          }
          if(i >= message_response.num_headers) {
            log4c_cdn(mycat, warn, "TRANSMIT", "Set-Cookie|Interface= not found, HTTP packet discarded");
            log4c_cdn(mycat, warn, "TRANSMIT", "Packet content is\n%s",buf);
            //exit(1);
          }
          /* get destination ID*/

          else {
            log4c_cdn(mycat, info, "TRANSMIT", "sending ==> connect=%d, length=%d", sockfd_connect,length);
            log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);
            length = write(sockfd_connect,buf,strlen(buf));
            handle_error_nn(length, 0, "TRANSMIT", "write() %s",strerror(errno));
          }
        }

      }
    }
    //monitor all sockfd_connect
    /*else*/ {
      for(sockfd_connect=0; sockfd_connect< FD_SETSIZE; sockfd_connect++) {
        if(!FD_ISSET(sockfd_connect, &readfds_temp)) continue; 
        if(sockfd_connect==sockfd_listen) continue;
        if(sockfd_connect==resfd) continue;

        memset(&message_request,  0, sizeof(message_request));
        memset(buf,0,sizeof(buf));

        length = read(sockfd_connect, buf, sizeof(buf)-1);
        handle_error_nn(length, 1,  "TRANSMIT", "read() from %d:%s",sockfd_connect,strerror(errno));
        log4c_cdn(mycat, info, "TRANSMIT", "receiving <== connect=%d length=%d", sockfd_connect, length);
        log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);

        if(length == 0) {
          ret = sd_close(sockfd_connect);
          handle_error_nn(ret, 1, "TRANSMIT","close() %s",strerror(errno));
          FD_CLR_P(sockfd_connect, &readfds);

          //HASH_ITER(hh, users, s, tmp) {
          //  if(s->id == sockfd_connect) {
          //    log4c_cdn(mycat, debug, "HASH", "entry %s=>%d removed from HASH", s->name, s->id);
          //    HASH_DEL( users, s);
          //    free(s);
          //  }
          //}
        }
        else if(length == (sizeof(buf)-1)) {
          log4c_cdn(mycat, error, "TRANSMIT", "http request is too large to store");
          exit(-1);
        }
        else {
          //deal with http packet

        /* get source ID */
          message_request.raw = buf;
          ret_parse=parse_messages(HTTP_REQUEST, 1, &message_request);
          switch(ret_parse) {
            case 1:
              log4c_cdn(mycat, warn, "TRANSMIT", "Packet unknown %d",ret_parse);
            case 0:
              url = calloc(1, strlen(message_request.request_url)+1);
              strcpy(url, message_request.request_url);
              key = basename(url);

//            printf("===============HASH=================\n");
//            HASH_ITER(hh, users, s, tmp) {
//              printf("%s:%d\n", s->name, s->id);
//            }

              //construct hash entry
              s = (struct my_struct*)calloc(1, sizeof(struct my_struct));
              s->id = sockfd_connect;
              strcpy(s->name, key);

              //put HTTP package & hash entry into queue
              INSERT_TAILQ_TAIL(svrinit)
              INSERT_TAILQ_TAIL(GetNodeSvrSysParmList)
              INSERT_TAILQ_TAIL(ReportTaskStatus)
              INSERT_TAILQ_TAIL(other)
insert_tailq_tail_end:
              free(url);
              break;
            case 2:
              INSERT_TAILQ_TAIL_EXTRA(svrinit)
              INSERT_TAILQ_TAIL_EXTRA(GetNodeSvrSysParmList)
              INSERT_TAILQ_TAIL_EXTRA(ReportTaskStatus)
              INSERT_TAILQ_TAIL_EXTRA(other)

              break;
            default:
              break;
          }
        }
      }
    }

    SEND_TAILQ_HEAD(svrinit)
    SEND_TAILQ_HEAD(GetNodeSvrSysParmList)
    SEND_TAILQ_HEAD(ReportTaskStatus)
    SEND_TAILQ_HEAD(other)

  }

}
