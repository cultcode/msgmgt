CFLAGS=-Wall -g

SRCS=ConcurrentStreamServer.c StreamClient.c main.c

HEADERS=ConcurrentStreamServer.h StreamClient.h main.h

TARGET=MsgAgentSvr
$(TARGET):$(SRCS) $(HEADERS)
	gcc -o $@ $(CFLAGS) $(SRCS) -DDEFAULT_DEBUGL=1 -lpthread

clean:
	rm -rf $(TARGET)
