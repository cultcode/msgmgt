#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "OperateXml.h"
#include "common.h"
#include "main.h"

int ReadConfigXml(char * fn_xml, char *** opt)
{
  xmlDocPtr doc=NULL;
  xmlNodePtr curNode=NULL;
  xmlChar* szAttr=NULL;
  int count=0;

  if(((*opt) = (char**)malloc(OPTIONS_LEN*sizeof(char*))) == NULL) {
    perror("malloc");
    exit(1);
  }
  memset((*opt), 0, OPTIONS_LEN*sizeof(char*));


  if(((*opt)[count] = malloc(OPTION_LEN)) == NULL) {
    perror("malloc");
    exit(1);
  }
  memset((*opt)[count], 0, OPTION_LEN);
  strcpy((*opt)[count],SelfBaseName);
  count++;

  //read file
  if ((doc = xmlReadFile(fn_xml,NULL,XML_PARSE_RECOVER)) == NULL) 
  {
    printf("Document %s not parsed successfully\n",fn_xml);     
    return count;
  } 
  else {
    printf("Be Noticed: reading configure arguments from %s......\n",fn_xml);
  }

  //get root node, e.g. <configuration/> and check error
  if (((curNode = xmlDocGetRootElement(doc)) == NULL) || (xmlStrcmp(curNode->name, BAD_CAST "configuration")))
  { 
    printf("wrong Node configuration\n"); 
    xmlFreeDoc(doc); 
    return count;
  } 

  //get <appSettings/> and check error
  curNode = curNode->children;
  while(curNode)
  {
    if((curNode->type == XML_ELEMENT_NODE) && (!xmlStrcmp(curNode->name, BAD_CAST "appSettings")))
    {
      break;
    }

    curNode = curNode->next;
  }

  if(!curNode)
  {
    printf("wrong Node appSettings\n"); 
    xmlFreeDoc(doc); 
    return count;
  }

  //get <add\> and check error
  curNode = curNode->children;
  for(;curNode;curNode = curNode->next)
  {
    if((curNode->type == XML_ELEMENT_NODE) && (!xmlStrcmp(curNode->name, BAD_CAST "add")))
    {
      if((szAttr = xmlGetProp(curNode,BAD_CAST "key")) == NULL) {
        printf("No attribute key\n");
        //count = -1;
        break;
      }

      if(((*opt)[count] = malloc(OPTION_LEN)) == NULL) {
        perror("malloc");
        exit(1);
      }
      memset((*opt)[count], 0, OPTION_LEN);

      if(strlen((char*)szAttr) == 1) {
        strcat((*opt)[count], "-");
      }
      else {
        strcat((*opt)[count], "--");
      }

      strcat((*opt)[count],(char*)szAttr);

      count++;
      xmlFree(szAttr);

      if((szAttr = xmlGetProp(curNode,BAD_CAST "value")) == NULL) {
        printf("No attribute value\n");
        //count = -1;
        break;
      }

      if(((*opt)[count] = malloc(OPTION_LEN)) == NULL) {
        perror("malloc");
        exit(1);
      }
      memset((*opt)[count], 0, OPTION_LEN);
      strcat((*opt)[count],(char*)szAttr);

      count++;
      xmlFree(szAttr);
    } 

  }

  xmlFreeDoc(doc);
  xmlCleanupParser();

  return count;
}

void usage() {
  printf(
    "-S  --server       :specify DBAgent domain/ip\n"
    "-i  --init         :specify SvrInit interface name, e.g. /ndas/NodeStatusInit\n"
    "-P  --portudp      :UDP port, default to 8942\n"
    "-O  --porttcp      :TCP port, default to 80\n"
    "-z  --zone         :specify timezone if necessary\n"
    "-s  --svrversion   :specify if field of version exists\n"
    "-q  --svrtype      :specify service ID\n"
    "-l  --logdir       :log directory\n"
    "-h  --help         :print this help info\n"
    "-v  --version      :print version info\n"
    );
}

int ParseOptions(int argc,char**argv)
{
  int  i=0;
  int flags=0;
  char* const short_options = "S:i:P:O:z:s:q:l:k:j:hv";
  struct option long_options[] = {  
    { "server",  1,  NULL,  'S'},  
    { "init",  1,  NULL,  'i'},  
    { "portudp",  1,  NULL,  'P'},  
    { "porttcp",  1,  NULL,  'O'},  
    { "zone",  1,  NULL,  'z'},  
    { "svrversion",  1,  NULL,  's'},  
    { "svrtype",  1,  NULL,  'q'},  
    { "logdir",  1,  NULL,  'l'},  
    { "des_key",  1,  NULL,  'k'},
    { "des_iv",  1,  NULL,  'j'},
    { "help",  0,  NULL,  'h'},  
    { "version",  0,  NULL,  'v'},  
    {  0,  0,  0,  0},  
  };

/********************************************************
 * reset all args value before get them
 ********************************************************/
  printf("number of arguments: %d\narguments: ",argc);
  for(i=0;i<argc;i++) {
    printf("%s ",argv[i]);
  }
  printf("\n");

/********************************************************
 * get input variables
 ********************************************************/
  optind = 1;
  while ( -1 != (i = getopt_long(argc, argv, short_options, long_options, NULL))) {
    switch (i) {
    case 'S':
      server = optarg;
      break;
    case 'i':
      url[0] = optarg;
      break;
    case 'P':
      port[UDP][REMOTE] = atoi(optarg);
      break;
    case 'O':
      port[TCP][REMOTE] = atoi(optarg);
      break;
    case 'z':
      servertimezone  = atoi(optarg);
      break;
    case 'h':
      usage();
      exit(0);
      break;
    case 'v':
      printf("%s Version %s\n",SelfBaseName, VERSION);
      exit(0);
      break;
    case 's':
      svrversion  = atoi(optarg);
      break;
    case 'q':
      svrtype  = atoi(optarg);
      break;
    case 'l':
      logdir  = optarg;
      break;
    case 'j':
      strcpy(node_3des_iv, optarg);
      break;
    case 'k':
      strcpy(node_3des_key, optarg);
      break;
    default:
      flags=1;
      break;
    }
  }

  char *str=NULL;
  char **pptr=NULL;
  struct hostent *hptr=NULL;

  str = calloc(IP_LEN, sizeof(char));

  if((server[0] >= '0') && (server[0] <= '9')) {
    if(ip[REMOTE][TCP]) free(ip[REMOTE][TCP]);
    if(ip[REMOTE][UDP]) free(ip[REMOTE][UDP]);
    ip[REMOTE][TCP] = ip[REMOTE][UDP] = server;
  }
  else {
    if( (hptr = gethostbyname(server) ) == NULL )
    {
      herror("gethostbyname()");
      fprintf(stderr,"ERROR: server:%s \n",server);
      exit(1);
    }
    switch(hptr->h_addrtype)
    {
    case AF_INET:
    case AF_INET6:
      pptr=hptr->h_addr_list;
      for(;*pptr!=NULL;pptr++) {
        if(inet_ntop(hptr->h_addrtype, *pptr, str, IP_LEN) == NULL) {
          perror("inet_ntop()");
          exit(1);
        }
      }
      break;
    default:
      printf("unknown address type/n");
      break;
    }
    if(ip[REMOTE][TCP]) free(ip[REMOTE][TCP]);
    if(ip[REMOTE][UDP]) free(ip[REMOTE][UDP]);
    ip[REMOTE][TCP] = ip[REMOTE][UDP] = str;
  }

  if(logdir) {
    mkdirs(logdir, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  printf("server: %s\ninit url: %s\nLOCAL TCP %s:%hd LOCAL UDP %s:%hd REMOTE TCP %s:%hd REMOTE UDP %s:%hd \ntime zone %d logdir %s\n",server, url[0], ip[LOCAL][TCP],port[TCP][LOCAL],ip[LOCAL][UDP],port[UDP][LOCAL],ip[REMOTE][TCP],port[TCP][REMOTE],ip[REMOTE][UDP],port[UDP][REMOTE],servertimezone,logdir);

return flags;

}

