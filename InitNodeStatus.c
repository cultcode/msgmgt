#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <assert.h>

#include "InitNodeStatus.h"
#include "cJSON.h"
#include "Security.h"

int svrversion=DEFAULT_SVRVERSION;
int svrtype=DEFAULT_SVRTYPE;
static int transfered;

__attribute__((weak)) int servertimezone=DEFAULT_SERVERTIMEZONE;

size_t readfunction( void *ptr, size_t size, size_t nmemb, void *userdata)
{
  int length = -1;
  char * cipher=NULL, content[CONTENT_LEN]={0};
  char *out=NULL;
  char EpochTime[16]={0};
  cJSON *root=NULL;
  struct NodeStatus * ns = (struct NodeStatus *)userdata;

  if(transfered) return 0;

  root=cJSON_CreateObject();

  sprintf(EpochTime,"%lx",ns->EpochTime);
  cJSON_AddStringToObject(root,"EpochTime",EpochTime);
  cJSON_AddStringToObject(root,"NodeName",ns->NodeName);
if(svrversion) {
  cJSON_AddStringToObject(root,"Version",ns->Version);
}
if(svrtype) {
  cJSON_AddNumberToObject(root,"SvrType",ns->SvrType);
}

  strcpy(content, out=cJSON_PrintUnformatted(root));

  cJSON_Delete(root);

  free(out);

  log4c_cdn(mycat, info, "POST", "%s",content);	      
  length = ContentEncode(NODE_3DES_KEY, NODE_3DES_IV, content, &cipher, strlen(content));
  if(length<0){
    log4c_cdn(mycat, error, "ENDEC", "ContentEncode() failed");	      
  }

  if(length<0) exit(1);

  memcpy(ptr,cipher,length);

  free(cipher);

  transfered=1;

  return length;
}

size_t writefunction( void *ptr, size_t size, size_t nmemb, void *userdata)
{
  int length = -1;
  char * plain = NULL;
  cJSON *root=NULL, *item=NULL;
  struct NodeStatus * ns = (struct NodeStatus *)userdata;

  if(size*nmemb == 0) return 0;

  length = ContentDecode(NODE_3DES_KEY, NODE_3DES_IV, ptr, &plain, StripNewLine(ptr, size*nmemb));
  log4c_cdn(mycat, info, "POST", "%s",plain);	      

  if(length<0) {
    log4c_cdn(mycat, error, "ENDEC", "ContentDecode() failed");	      
    exit(1);
  }

  if((root = cJSON_Parse(plain)) == NULL) {
    log4c_cdn(mycat, error, "JSON", "cJSON_Parse() failed: %s",cJSON_GetErrorPtr());	      
    exit(1);
  }

  item = cJSON_GetObjectItem(root,"Status");
  ns->Status = item->valueint;

  item = cJSON_GetObjectItem(root,"StatusDesc");
  strcpy(ns->StatusDesc, item->valuestring);

  if(ns->Status == SUCCESS) {
    item = cJSON_GetObjectItem(root,"NodeId");
    ns->NodeId = item->valueint;
  }

  cJSON_Delete(root);
  
  if(ns->Status == FAIL) {
    log4c_cdn(mycat, error, "POST", "FAIL received");	      
    exit(1);
  }

  free(plain);
  return size*nmemb;
}  

void InitNodeStatus()
{
  CURL *curl=NULL;
  CURLcode res;
  struct NodeStatus ns={0};

  memset(&ns,0,sizeof(struct NodeStatus));
/*structure http request
{"EpochTime":" 97d76a","NodeName":"Imgo-1","Version":"3.0"}
*/
  ns.EpochTime = GetLocaltimeSeconds(servertimezone);

  gethostname(ns.NodeName,NODENAME_LEN);

  sprintf(ns.Version,"%s",VERSION);
  ns.SvrType = svrtype;

  char posturl[URL_LEN]={0};
  sprintf(posturl,"http://%s:%hd%s",server,port[REMOTE][TCP],url[0]);

  struct curl_slist *header = NULL;
  header = curl_slist_append(header, "Transfer-Encoding: chunked");
  header = curl_slist_append(header, "Connection: Close");

  transfered=0;

  curl = curl_easy_init();  
  curl_easy_setopt(curl, CURLOPT_URL, posturl);  
  curl_easy_setopt(curl, CURLOPT_READDATA, &ns);  
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, readfunction);  
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ns);  
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunction);  
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);  
  curl_easy_setopt(curl, CURLOPT_POST, 1);  
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt(curl, CURLOPT_HEADER, 0);  
  curl_easy_setopt(curl, CURLOPT_NOBODY, 0);  
  res = curl_easy_perform(curl);  
  curl_easy_cleanup(curl);  
  curl_slist_free_all(header);

  assert(res == CURLE_OK);

}

