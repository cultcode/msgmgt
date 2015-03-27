CFLAGS=-Wall -g

SRCS=ConcurrentServer.c main.c

HEADERS=ConcurrentServer.h main.h

TARGET=MsgAgentSvr
$(TARGET):$(SRCS) $(HEADERS)
	gcc -o $@ $(CFLAGS) $(SRCS) -DDEFAULT_DEBUGL=1

