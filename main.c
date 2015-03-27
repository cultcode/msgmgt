#include <stdio.h>
#include <sys/socket.h>
#include "ConcurrentServer.h"

int debugl = DEFAULT_DEBUGL;

int main(int argc, char **argv)
{
ConcurrentServer("127.0.0.1", 7070, SOCK_STREAM);
return 0;
}
