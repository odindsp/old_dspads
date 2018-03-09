#include <sys/stat.h>
#include <iostream>
#include "toutiao_adapter.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include <stdlib.h>
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include <signal.h>
#include <syslog.h>
#include "toutiao_ssp_api.pb.h"

using namespace std;

#define CONFIG_FILE          "/etc/dspads/dsp_toutiao.conf"

uint64_t g_logid, g_logid_local;
char *g_server_ip = NULL;
uint32_t g_server_port;
uint64_t ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;

static void *doit(void *arg)
{
  uint8_t index = (uint64_t)arg;
  DATATRANSFER *datatransfer = new DATATRANSFER;
  pthread_t threadlog;
  FCGX_Request request;
  pthread_detach(pthread_self());
  FCGX_InitRequest(&request, 0, 0);
  int rc;

  if (datatransfer == NULL)
  {
     writeLog(g_logid_local,LOGERROR, "thread = %d allocate datatransfer failed!",index);
     return NULL;
  }

  datatransfer->server = g_server_ip;
  datatransfer->port = g_server_port;
  datatransfer->logid = g_logid_local;

connect_log_server:
  datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
  if (datatransfer->fd == -1)
  {
    writeLog(g_logid_local,LOGERROR , "Connect log server " + datatransfer->server + " failed!");
    usleep(100000);
    goto connect_log_server;
  }
 else
   va_cout("connect success");

  pthread_mutex_init(&datatransfer->mutex_data, 0);
  pthread_create(&threadlog, NULL, threadSendLog, datatransfer);

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
       uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
       if (contentlength == 0)
       {
          writeLog(g_logid_local,LOGERROR , "recvdata length is zero!");
          continue;
       }

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
       writeLog(g_logid_local, LOGDEBUG,"recvdata = "+string(recvdata->data,recvdata->length));
       string senddata;
       toutiao_adapter(ctx,index,datatransfer,recvdata,senddata);
       if (senddata.length() > 0)
       {
         pthread_mutex_lock(&counts_mutex);
         FCGX_PutStr(senddata.data(),senddata.length(), request.out);
         pthread_mutex_unlock(&counts_mutex);
       }
     }
     else
     {
       va_cout("Not POST.");
       writeLog(g_logid_local, LOGERROR ,"Not POST.");
     }
     FCGX_Finish_r(&request);
  }
  disconnectServer(datatransfer->fd);
  pthread_mutex_destroy(&datatransfer->mutex_data);
  delete datatransfer;
  return NULL;
}

void sigroutine(int dunno)
{
  switch (dunno)
  {
    case SIGINT:
    case SIGTERM:
          if(dunno == SIGINT)
            syslog(LOG_INFO,"recv sigint");
           else
            syslog(LOG_INFO,"recv sigterm");
   
          va_cout("Get a signal -- %d, ctx:%d, cpu_count:%d",dunno, ctx, cpu_count);

             if (ctx != 0)
             {
                int errcode = bid_filter_uninitialize(ctx);
                va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
                ctx = 0;

                for (int i = 0; i < cpu_count; ++i)
                {
                   va_cout("pthread_kill[%d]:%d...", i, id[i]);
                   pthread_kill(id[i], SIGKILL);
  //                 pthread_cancel(id[i]);
                   va_cout("pthread_kill[%d]:%d over", i, id[i]);
                }
              }
              break;
    default:
              cout<<"Get a unknown -- "<< dunno << endl;
              break;
  }

  return;
}

int main(int argc, char *argv[])
{
   int err = 0;
   void *status = NULL;
   bool parsesuccess = false;
   string logdir;
   bool is_textdata = false;

   FCGX_Init();
   cpu_count = GetPrivateProfileInt(GLOBAL_CONF,"default","cpu_count");
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

   signal(SIGINT, sigroutine);
   signal(SIGTERM,sigroutine);

   g_server_ip = GetPrivateProfileString(CONFIG_FILE,"logserver","bid_ip");
   if (g_server_ip == NULL)
   {
     cflog(g_logid_local, LOGERROR, "GetPrivateProfileString failed! name:%s, section:%s, key:%s",CONFIG_FILE, "logserver", "bid_ip");
     goto release;
   }

   g_server_port = GetPrivateProfileInt(CONFIG_FILE,"logserver","bid_port");
   if (g_server_port == 0)
   {
     cflog(g_logid_local, LOGERROR, "GetPrivateProfileInt failed! name:%s, section:%s, key:%s",CONFIG_FILE, "logserver", "bid_port");
     goto release;
   }
   va_cout("serverip=%s",g_server_ip);
   va_cout("serverport=%d",g_server_port);

   logdir = "loglocal";
   is_textdata = GetPrivateProfileInt(CONFIG_FILE,(char *)(logdir.c_str()), "is_textdata");
   va_cout("loglocal is_textdata  %d", is_textdata);
   g_logid_local = openLog(CONFIG_FILE,(char *)(logdir.c_str()),is_textdata);
   if(g_logid_local == 0)
   {
     va_cout("open local log error");
     goto release;
   }

   logdir = "log";
   is_textdata = GetPrivateProfileInt(CONFIG_FILE,(char *)(logdir.c_str()), "is_textdata");
   va_cout("log is_textdata  %d", is_textdata);
   g_logid = openLog(CONFIG_FILE,(char *)(logdir.c_str()),is_textdata);
   if(g_logid == 0)
   {
     va_cout("open log error");
     goto release;
   }
   fullreqrecord = GetPrivateProfileInt(CONFIG_FILE, (char *)(logdir.c_str()), "fullreqrecord");

   err = bid_filter_initialize(g_logid_local, ADX_TOUTIAO, cpu_count, &ctx);
   if (err != 0)
   {
     writeLog(g_logid_local,LOGERROR, "bid filter initialize error");
     goto release;
   }

   id[0] = pthread_self();
   for (uint64_t i = 1; i < cpu_count; ++i)
   {
     pthread_create(&id[i], NULL, doit, (void *)i);
   }
   doit(0);

release:
   if (id != NULL)
   {
       delete []id;
       id = NULL;
   }
   if (g_logid != 0)
      closeLog(g_logid);
   if (g_logid_local != 0)
      closeLog(g_logid_local);
   if (g_server_ip != NULL)
      free(g_server_ip);
   if (ctx != 0)
   {
      int errcode = bid_filter_uninitialize(ctx);
      va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
      ctx = 0;
   }
  google::protobuf::ShutdownProtobufLibrary();  
  cout<<"main end"<<endl; 
   return 0;
}

