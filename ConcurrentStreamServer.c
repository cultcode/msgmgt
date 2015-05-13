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
    UT_hash_handle hh;         /* makes this structure hashable */
};
static struct my_struct *users=NULL;

struct tailq_entry_svrinit {
    char value[HTTP_LEN];
    struct my_struct *record;
    TAILQ_ENTRY(tailq_entry_svrinit) tailq_entry;
};
TAILQ_HEAD(, tailq_entry_svrinit) tailq_svrinit_head;

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
  ip_t ip_local = ip[TCP][LOCAL];
  port_t port_local = port[TCP][LOCAL];
  struct timeval timeout={10,0}, timeout_temp={0};
  int ret=0;
  //int nPipeReadFlag = 0;
  //int pipefd_so[2]={-1};
  int i=0;
  struct my_struct *s=NULL, *tmp=NULL;
  char *key=NULL;
  char *url=NULL, *cookie=NULL;
  struct tailq_entry_svrinit *item;

  TAILQ_INIT(&tailq_svrinit_head);

  reqfd = ((int*)pipefd)[PIPE_INDEX(REQUEST,WRITE)];
  resfd = ((int*)pipefd)[PIPE_INDEX(RESPONSE,READ)];

//  ret = pipe(pipefd_so);
//  handle_error_nn(ret, "pipe()");
//
//  nPipeReadFlag = fcntl(pipefd_so[0], F_GETFL, 0);
//  nPipeReadFlag |= O_NONBLOCK;
//
//  ret = fcntl(pipefd_so[0], F_SETFL, nPipeReadFlag);
//  handle_error_nn(ret, "fcntl()");

  addr_mine.sin_family = AF_INET;
  inet_pton(AF_INET,ip_local,&addr_mine.sin_addr);
  addr_mine.sin_port = htons(port_local);

  sockfd_listen = socket(AF_INET, SOCK_STREAM, 0);
  handle_error_nn(sockfd_listen, "TRANSMIT", "socket()");

  ret = bind(sockfd_listen, (struct sockaddr *)&addr_mine, sizeof(struct sockaddr));
  handle_error_nn(ret, "TRANSMIT", "bind()");

  ret = listen(sockfd_listen, 10);
  handle_error_nn(ret, "TRANSMIT", "listen()");

  FD_ZERO(&readfds);

  FD_SET_P(sockfd_listen, &readfds);

  FD_SET_P(resfd, &readfds);

  while(1) {
    memset(&message_response, 0, sizeof(message_response));
    memset(&message_request,  0, sizeof(message_request));

    timeout_temp = timeout;
    readfds_temp = readfds;

    ret = select(FD_SETSIZE, &readfds_temp, NULL, NULL, NULL);
    handle_error_nn(ret, "TRANSMIT", "ConcurrentStreamServer() select()");

    if(ret == 0) {
      log4c_cdn(mycat, error, "TRANSMIT", "select() timeout");
      exit(-1);
    }

    //monitor sockfd_listen
    if(FD_ISSET(sockfd_listen, &readfds_temp)) {
      sockfd_connect = accept(sockfd_listen, (struct sockaddr *)NULL, NULL);
      handle_error_nn(sockfd_connect, "TRANSMIT", "accept()");

      FD_SET_P(sockfd_connect, &readfds);
    }
    //monitor response fd
    else if(FD_ISSET(resfd, &readfds_temp)) {

      memset(buf,0,sizeof(buf));
      length = read(resfd, buf, sizeof(buf)-1);
      log4c_cdn(mycat, debug, "TRANSMIT", "receiving packet, source=resfd, sockfd=%d length=%d", resfd, length);
      handle_error_nn(length, "TRANSMIT", "read()");

      if(length == 0) {
        close(resfd);
        FD_CLR_P(resfd, &readfds);
      }
      else {
        //deal with http packet
        log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);

        message_response.raw = buf;
        parse_messages(HTTP_RESPONSE, 1, &message_response);

        for(i=0; i<message_response.num_headers; i++) {
          if(!strcasecmp(message_response.headers[i][0], "Set-Cookie" )) {

            cookie = calloc(1, strlen(message_response.headers[i][1])+1);
            strcpy(cookie,message_response.headers[i][1]);
            key = strstr(cookie,"Interface=");
            key += strlen("Interface=");

            HASH_FIND_STR( users, key, s);

            if(s == NULL) {
              log4c_cdn(mycat, error, "HASH", "%s=>NULL found from hash", key);
              exit(1);
            }
            else {
              sockfd_connect = s->id;
              log4c_cdn(mycat, debug, "HASH", "%s=>%d found from hash",key, s->id);
            }


            if(!strcasecmp(key,"svrinit")) {
              item = TAILQ_FIRST(&tailq_svrinit_head);
              log4c_cdn(mycat, debug, "QUEUE", "entry removed from TAILQ svrinit");
              TAILQ_REMOVE(&tailq_svrinit_head, item, tailq_entry);
            }

            free(cookie);

            break;
          }
        }

        if(i >= message_response.num_headers) {
          log4c_cdn(mycat, error, "TRANSMIT", "set-cookie not found, HTTP packet discarded");
          log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",message_response.raw);
          //exit(1);
        }
        /* get destination ID*/

        else {
          length = write(sockfd_connect,buf,strlen(buf));
          log4c_cdn(mycat, info, "TRANSMIT", "sending packet, destination=connect, sockfd=%d, length=%d", sockfd_connect,length);
          handle_error_nn(length, "TRANSMIT", "write()");
        }
      }
    }
    //monitor all sockfd_connect
    else {
      for(sockfd_connect=0; sockfd_connect< FD_SETSIZE; sockfd_connect++) {
        if(!FD_ISSET(sockfd_connect, &readfds_temp)) continue; 

        memset(buf,0,sizeof(buf));
        length = read(sockfd_connect, buf, sizeof(buf)-1);
        log4c_cdn(mycat, info, "TRANSMIT", "receiving packet, source=connect, sockfd=%d length=%d", sockfd_connect, length);
        handle_error_nn(length, "TRANSMIT", "read()");

        if(length == 0) {
          close(sockfd_connect);
          FD_CLR_P(sockfd_connect, &readfds);

          HASH_ITER(hh, users, s, tmp) {
            if(s->id == sockfd_connect) {
              log4c_cdn(mycat, debug, "HASH", "entry %s=>%d removed from HASH", s->name, s->id);
              HASH_DEL( users, s);
              free(s);
            }
          }
        }
        else if(length == (sizeof(buf)-1)) {
          log4c_cdn(mycat, error, "TRANSMIT", "http request is too large to store");
          exit(-1);
        }
        else {
          //deal with http packet
        log4c_cdn(mycat, debug, "TRANSMIT", "Packet content is\n%s",buf);

        /* get source ID */
          message_request.raw = buf;
          parse_messages(HTTP_REQUEST, 1, &message_request);

          url = calloc(1, strlen(message_request.request_url)+1);
          strcpy(url, message_request.request_url);
          key = basename(url);

//          printf("===============HASH=================\n");
//          HASH_ITER(hh, users, s, tmp) {
//            printf("%s:%d\n", s->name, s->id);
//          }

          //construct hash entry
          s = (struct my_struct*)calloc(1, sizeof(struct my_struct));
          s->id = sockfd_connect;
          strcpy(s->name, key);

          //put HTTP package & hash entry into queue
          if(!strcasecmp(key, "svrinit")) {
            item = calloc(1, sizeof(struct tailq_entry_svrinit));
            strcpy(item->value, buf);
            item->record = s;
            TAILQ_INSERT_TAIL(&tailq_svrinit_head, item, tailq_entry);
            log4c_cdn(mycat, debug, "QUEUE", "entry added into TAILQ svrinit");
          }
          else {
            //HASH_REPLACE_STR(head,keyfield_name, item_ptr, replaced_item_ptr)
            HASH_REPLACE_STR( users, name, s, tmp );
            free(tmp);
            log4c_cdn(mycat, debug, "HASH", "%s added into HASH", s->name);

            length = write(reqfd, buf, sizeof(buf)-1);
            log4c_cdn(mycat, debug, "TRANSMIT", "sending packet, destination=reqfd, sockfd=%d, length=%d", reqfd,length);
            handle_error_nn(length, "TRANSMIT", "write()");
          }

//          length = write(pipefd_so[1],&sockfd_connect,sizeof(sockfd_connect));
//          handle_error_nn(length, "TRANSMIT", "write()");

          free(url);
        }
      }
    }

    item = TAILQ_FIRST(&tailq_svrinit_head);
    if((item != NULL) && (TAILQ_NEXT(item, tailq_entry) == NULL))  {
      s = item->record;

      HASH_REPLACE_STR( users, name, s, tmp);
      free(tmp);
      log4c_cdn(mycat, debug, "HASH", "entry %s=>%d added into HASH", s->name, s->id);

      length = write(reqfd, item->value, sizeof(buf)-1);
      log4c_cdn(mycat, debug, "TRANSMIT", "sending packet, destination=reqfd, sockfd=%d, length=%d", reqfd,length);
      handle_error_nn(length, "TRANSMIT", "write()");
    }
  }

}
