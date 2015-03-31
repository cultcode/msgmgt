#ifndef CONCURRENTSERVER_H
#define CONCURRENTSERVER_H

extern char *ip_local;
extern short port_local;

extern void* ConcurrentStreamServer(void *pipefd);

#endif
