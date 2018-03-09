#include <iostream>
#include "toutiao_adapter.h"
#include "../../common/type.h"
#include "../../common/confoperation.h"
#include <stdlib.h>
using namespace std;

#define CONFIG_FILE          "/etc/dspads/dsp_toutiao.conf"

uint8_t cpu_count = 0;
pthread_t *id = NULL;

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
  //  va_cout("remoteaddr:%s",remoteaddr);

     if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
     {
       if(strcmp(FCGX_GetParam("CONTENT_TYPE", request.envp),"application/octet-stream") != 0)
       {
	     continue;
       }
       uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
       if(contentlength == 0)
       {
          continue;
       }

       //va_cout("contentlength:%d",contentlength);
       RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
       if (recvdata == NULL)
       {
	    continue;
       }

       recvdata->length = contentlength;
       recvdata->data = (char *)calloc(1, recvdata->length + 1);
       if (recvdata->data == NULL)
       {
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
	   continue;
       }

	  va_cout("recvlen:%d",recvdata->length);
	  va_cout("recvdata:%s",recvdata->data);
      string senddata;
      toutiao_adapter(recvdata, senddata);

    if(senddata.length() > 0)
       {
         pthread_mutex_lock(&counts_mutex);
         FCGX_PutStr(senddata.data(),senddata.length(), request.out);
         pthread_mutex_unlock(&counts_mutex);
       }
     }
     else
     {
     //   va_cout("Not POST.");
     }
     FCGX_Finish_r(&request);
  }

  return NULL;
}

int main(int argc, char *argv[])
{
   bool parsesuccess = false;

   FCGX_Init();
   cpu_count = GetPrivateProfileInt(GLOBAL_CONF,"default","cpu_count");
   if (cpu_count < 1)
   {
    //  va_cout("cpu_count error!");
      goto release;
   }

   id = new pthread_t[cpu_count];
   if(id == NULL)
   {
   // va_cout("allocate pthread_t error");
   //  va_cout("cpu_count=%d",cpu_count);
    goto release;
   }
  // va_cout("cpu_count=%d",cpu_count);

   id[0] = pthread_self();
   for (uint8_t i = 1; i < cpu_count; ++i)
   {
      pthread_create(&id[i], NULL, doit, (void *)i);
   }
	doit(0);

release:
   if(id != NULL)
      delete []id;
   cout<<"main end"<<endl;
   return 0;
}

