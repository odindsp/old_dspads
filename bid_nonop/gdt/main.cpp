#include <sys/stat.h>
#include <iostream>
#include "../../common/tcp.h"
#include "../../common/util.h"
#include <stdlib.h>
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <map>
#include <vector>
#include "../../common/confoperation.h"

#include <signal.h>
#include <syslog.h>

#include "../gdt/gdt_adapter.h"
#include "../gdt/gdt_rtb.pb.h"
using namespace std;

#define PRIVATE_CONF         "dsp_gdt.conf"

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

    MY_DEBUG
    pthread_mutex_lock(&accept_mutex);
    MY_DEBUG
    rc = FCGX_Accept_r(&request);
    MY_DEBUG
    pthread_mutex_unlock(&accept_mutex);

    MY_DEBUG
    if (rc < 0)
	     break;

    MY_DEBUG
    char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);

    char* method = FCGX_GetParam("REQUEST_METHOD", request.envp);

     if (strcmp("POST", method) == 0)
     {
       if(strcmp(FCGX_GetParam("CONTENT_TYPE", request.envp),"application/x-protobuf") != 0)
       {
         goto finish;
       }
       uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
       if(contentlength == 0)
       {
          goto finish;
       }

       RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
       if (recvdata == NULL)
       {
        goto finish;
       }
       MY_DEBUG

       recvdata->length = contentlength;
       recvdata->data = (char *)calloc(1, recvdata->length + 1);
       if (recvdata->data == NULL)
       {
         free(recvdata);
         recvdata = NULL;
         goto finish;
       }
       if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
       {
	     free(recvdata->data);
	     free(recvdata);
             recvdata->data = NULL;
             recvdata = NULL;
	     goto finish;
       }


       string senddata;
       gdt_adapter(recvdata, senddata);

       if(senddata.length() > 0)
       {
         pthread_mutex_lock(&counts_mutex);
         FCGX_PutStr(senddata.data(),senddata.length(), request.out);
         pthread_mutex_unlock(&counts_mutex);
         //va_cout("put");
       }
     }
     else
     {
        //va_cout("Not POST.");
     }
     
     finish:
	 	 FCGX_Finish_r(&request);
  }

exit:
  va_cout("doit exit");
  return NULL;
}

int main(int argc, char *argv[])
{
   int err = E_SUCCESS;
   	
   string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
   char *global_conf = (char *)str_global_conf.c_str();

   vector<pthread_t> thread_id;

   FCGX_Init();

   cpu_count = GetPrivateProfileInt(global_conf,"default","cpu_count");
   if (cpu_count < 1)
   {
	    FAIL_SHOW
	  err = E_INVALID_CPU_COUNT;
      //va_cout("cpu_count error!");
      goto release;
   }

  id = new pthread_t[cpu_count];
   if(id == NULL)
   {
	  // va_cout("allocate pthread_t error");
	  // va_cout("cpu_count=%d",cpu_count);
	   FAIL_SHOW
	   err = E_MALLOC;
	   goto release;
   }
   //va_cout("cpu_count=%d",cpu_count);

	id[0] = pthread_self();
	for (uint8_t i = 1; i < cpu_count; ++i)
	{

		pthread_create(&id[i], NULL, doit, (void *)i);
	}
	doit(0);


release:

    google::protobuf::ShutdownProtobufLibrary();

    if(id != NULL)
    {
	   delete []id;
	   id = NULL;
    }
	va_cout("main  exit.");
    return err;
}

