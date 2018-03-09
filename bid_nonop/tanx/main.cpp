#include <sys/stat.h>
#include <iostream>
#include "tanx_adapter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "tanx-bidding.pb.h"

using namespace std;

#define PRIVATE_CONF          "dsp_tanx.conf"

uint64_t g_logid_local;
uint64_t ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;

static void *doit(void *arg)
{
  FCGX_Request request;
  pthread_detach(pthread_self());
  FCGX_InitRequest(&request, 0, 0);
  int rc;

  while(1)
  {
    static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&accept_mutex); 
    rc = FCGX_Accept_r(&request);
    pthread_mutex_unlock(&accept_mutex);
    
    if (rc < 0)
	     break;

    char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
    va_cout("remoteaddr:%s",remoteaddr);
   
     if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
     { 
       if(strcmp(FCGX_GetParam("CONTENT_TYPE", request.envp),"application/octet-stream") != 0)
       {
         writeLog(g_logid_local,LOGERROR,"Content-Type error.");
	     continue;
       }
       uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
       if(contentlength == 0)
       {
          writeLog(g_logid_local,LOGERROR , "recvdata length is zero!");
          continue;
       }
       
       va_cout("contentlength:%d",contentlength);
       RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
       if (recvdata == NULL)
       {
        writeLog(g_logid_local,LOGERROR , "recvdata:alloc memory failed!");
	    continue;
       }

       recvdata->length = contentlength;
       recvdata->data = (char *)calloc(1, recvdata->length + 1);
       if (recvdata->data == NULL)
       {
         writeLog(g_logid_local,LOGERROR , "recvdata->data:alloc memory failed!");
	     free(recvdata);
         recvdata = NULL;
         continue;
       }
       if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
       {
	       free(recvdata->data);
	       free(recvdata);
           recvdata->data = NULL;
           recvdata = NULL;
	       writeLog(g_logid_local,LOGERROR , "fread fail");
	       continue;
       }
        
	  va_cout("recvlen:%d",recvdata->length);
	  va_cout("recvdata:%s",recvdata->data);
      writeLog(g_logid_local, LOGINFO,"recvdata = "+string(recvdata->data,recvdata->length));
      string senddata;
       tanx_adapter(recvdata, senddata);
 
    if(senddata.length() > 0)
       { 
         pthread_mutex_lock(&counts_mutex);
         FCGX_PutStr(senddata.data(),senddata.length(), request.out);
         pthread_mutex_unlock(&counts_mutex);
       }
     }
     else
     {
        va_cout("Not POST.");
        writeLog(g_logid_local,LOGERROR , "Not POST.");
     }
     FCGX_Finish_r(&request);
  }
 
  return NULL;
}

int main(int argc, char *argv[])
{ 
   bool parsesuccess = false;
   string logdir;
   bool is_textdata = false;
   string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	 char *global_conf = (char *)str_global_conf.c_str();
	 string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	 char *private_conf = (char *)str_private_conf.c_str();

   FCGX_Init();

   cpu_count = GetPrivateProfileInt(global_conf,"default","cpu_count");
   if (cpu_count < 1)
   {
      va_cout("cpu_count error!");
      goto release;
   }   

   id = new pthread_t[cpu_count];
   if(id == NULL)
   {
    va_cout("allocate pthread_t error");
    va_cout("cpu_count=%d",cpu_count);
    goto release;
   }
   va_cout("cpu_count=%d",cpu_count);
  
   logdir = "loglocal";
   is_textdata = GetPrivateProfileInt(private_conf,(char *)(logdir.c_str()), "is_textdata");
   va_cout("loglocal is_textdata  %d", is_textdata);
   g_logid_local = openLog(private_conf,(char *)(logdir.c_str()),is_textdata);
   if(g_logid_local == 0)
   {
     va_cout("open local log error");
     goto release;
   }


   id[0] = pthread_self();
   for (uint8_t i = 1; i < cpu_count; ++i)
   {
      pthread_create(&id[i], NULL, doit, (void *)i);
   }

	doit(0);
 
release:
   if(id != NULL)
      delete []id;
   if(g_logid_local != 0)
      closeLog(g_logid_local);
        
   cout<<"main end"<<endl; 
   google::protobuf::ShutdownProtobufLibrary();
   return 0;
}

