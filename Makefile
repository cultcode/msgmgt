CFLAGS=-Wall -g

SRCS=ConcurrentStreamServer.c StreamClient.c RoundGramServer.c main.c

HEADERS=ConcurrentStreamServer.h StreamClient.h RoundGramServer.h main.h common.h

TARGET=MsgAgentSvr
$(TARGET):$(SRCS) $(HEADERS)
	gcc -o $@ $(CFLAGS) $(SRCS) -DDEFAULT_DEBUGL=3 -lpthread

clean:
	rm -rf $(TARGET)
