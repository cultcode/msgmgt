#ifndef CONCURRENTSERVER_H
#define CONCURRENTSERVER_H

extern void* ConcurrentStreamServer(void *pipefd);

#define DEFINE_TAILQ(type) \
struct tailq_entry_##type { \
    char value[HTTP_LEN]; \
    char extra[HTTP_LEN]; \
    struct my_struct *record; \
    TAILQ_ENTRY(tailq_entry_##type) tailq_entry; \
}; \
TAILQ_HEAD(headname_##type, tailq_entry_##type) tailq_##type##_head;

#define REMOVE_TAILQ(type) \
  if(!strcasecmp(key,#type)) { \
    item_##type = TAILQ_FIRST(&tailq_##type##_head); \
    log4c_cdn(mycat, debug, "QUEUE", "entry removed from TAILQ %s",#type); \
    TAILQ_REMOVE(&tailq_##type##_head, item_##type, tailq_entry); \
  }

#define INSERT_TAILQ_TAIL(type) \
  if(!strcasecmp(key, #type) || !strcmp(#type,"other")) { \
    item_##type = calloc(1, sizeof(struct tailq_entry_##type)); \
    strcpy(item_##type->value, buf); \
    item_##type->record = s; \
    TAILQ_INSERT_TAIL(&tailq_##type##_head, item_##type, tailq_entry); \
    log4c_cdn(mycat, debug, "QUEUE", "entry added into TAILQ %s",#type); \
    goto insert_tailq_tail_end; \
  }

#define INSERT_TAILQ_TAIL_EXTRA(type) \
  if(((item_##type = TAILQ_LAST(&tailq_##type##_head, headname_##type)) != NULL) && ((s = item_##type->record) != NULL) && (s->id == sockfd_connect)){ \
    if(!s->sent && !strlen(item_##type->extra)) { \
      strcpy(item_##type->extra, buf); \
      log4c_cdn(mycat, debug, "QUEUE", "entry extra added into TAILQ %s",#type); \
    } \
    else { \
      log4c_cdn(mycat, error, "TRANSMIT", "HTTP content left alone in QUEUE %s",#type); \
      exit(1); \
    } \
  }

#define SEND_TAILQ_HEAD(type) \
    item_##type = TAILQ_FIRST(&tailq_##type##_head); \
    if((item_##type != NULL) && ((s = item_##type->record) != NULL) && !s->sent && !((strlen(strstr(item_##type->value,"\r\n\r\n")) == 4) && !strlen(item_##type->extra)))  { \
      HASH_REPLACE_STR( users, name, s, tmp); \
      free(tmp); \
      log4c_cdn(mycat, debug, "HASH", "entry %s=>%d added into HASH", s->name, s->id); \
 \
      if(!strlen(item_##type->extra) && strstr(item_##type->value,"Expect:")) { \
        token = strdup(item_##type->value); \
        strrpl(item_##type->value, token, "Expect: 100-continue\r\n", ""); \
        free(token); \
      } \
\
      if(strlen(item_##type->value)) { \
        length = write(reqfd, item_##type->value, strlen(item_##type->value)); \
        handle_error_nn(length, 1, "TRANSMIT", "write() %s",strerror(errno)); \
        log4c_cdn(mycat, info, "TRANSMIT", "sending ==> reqfd=%d, length=%d", reqfd,length); \
      } \
\
      if(strlen(item_##type->extra)) { \
        length = write(reqfd, item_##type->extra, strlen(item_##type->extra)); \
        handle_error_nn(length, 1, "TRANSMIT", "write() %s",strerror(errno)); \
        log4c_cdn(mycat, info, "TRANSMIT", "sending ==> reqfd=%d, length=%d", reqfd,length); \
      } \
\
      s->sent = 1; \
      if(!strcmp(#type,"other")) TAILQ_REMOVE(&tailq_##type##_head, item_##type, tailq_entry); \
    }

#endif
