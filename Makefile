CFLAGS=-Wall -g

HASHDIR = ../uthash/src
CFLAGS += -I$(HASHDIR)

SRCS=http_parser.c http_parser_package.c ConcurrentStreamServer.c StreamClient.c RoundGramServer.c main.c

HEADERS=$(HASHDIR)/uthash.h http_parser.h http_parser_package.h ConcurrentStreamServer.h StreamClient.h RoundGramServer.h main.h common.h

TARGET=MsgAgentSvr
$(TARGET):$(SRCS) $(HEADERS)
	gcc $(CFLAGS) -o $@ $(SRCS) -DDEFAULT_DEBUGL=3 -lpthread -ldl

clean:
	rm -rf $(TARGET)
