#ifndef STREAMCLIENT_H
#define STREAMCLIENT_H

extern int debugl;
extern ip_t *ip;
extern port_t *port;

extern void * StreamClient(void *pipefd);

#endif
