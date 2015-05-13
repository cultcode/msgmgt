VERSION=\"1.0.$(shell date +%Y%m%d).$(shell date +%H%M%S)\"

CFLAGS=-Wall -g

HASHDIR = ../uthash/src
CFLAGS += -I$(HASHDIR) -I/usr/include/libxml2 -I./openssl/include -L./openssl -DDEFAULT_DEBUGL=0

SRCS=http_parser.c http_parser_package.c cJSON.c common.c Security.c OperateXml.c InitNodeStatus.c ConcurrentStreamServer.c StreamClient.c RoundGramServer.c main.c

HEADERS=$(HASHDIR)/uthash.h http_parser.h http_parser_package.h cJSON.h common.h Security.h OperateXml.h InitNodeStatus.h ConcurrentStreamServer.h StreamClient.h RoundGramServer.h main.h common.h

TARGET=NodeMsgManageSvr
$(TARGET):$(SRCS) $(HEADERS)
	gcc $(CFLAGS) -o $@ $(SRCS) -DVERSION="$(VERSION)" -lpthread -ldl -lxml2 -lcurl -lcrypto

clean:
	rm -rf $(TARGET)
