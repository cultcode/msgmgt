#ifndef INITNODESTATUS_H
#define INITNODESTATUS_H

#include "common.h"

#define DEFAULT_SVRTYPE 0
#define DEFAULT_SVRVERSION 0

enum {
  FAIL=0,
  SUCCESS=1
};

extern int svrversion;
extern int svrtype;

struct NodeStatus {
  long  EpochTime;
  char  NodeName[NODENAME_LEN];
  char  Version[VERSION_LEN];
  int   SvrType;
  int   Status;/*1:success 0:failure*/
  char  StatusDesc[STATUSDESC_LEN];
  int   NodeId;
};

extern void InitNodeStatus();

#endif
