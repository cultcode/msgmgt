#ifndef CONCURRENTSERVER_H
#define CONCURRENTSERVER_H

extern int debugl;
extern ip_t *ip;
extern port_t *port;

extern void* ConcurrentStreamServer(void *pipefd);

#endif
