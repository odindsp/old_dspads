#include <iostream>
#include "momo_adapter.h"
#include <stdlib.h>
#include "../../common/confoperation.h"
#include "momortb12.pb.h"
using namespace std;

uint8_t cpu_count = 0;
pthread_t *id = NULL;

static void *doit(void *arg)
{
  FCGX_Request request;
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
	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
	{
		/*if(strcmp(FCGX_GetParam("CONTENT_TYPE", request.envp),"application/x-protobuf") != 0)
		{
			goto finish;
		}*/
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
		momo_adapter(recvdata, senddata);

		if(senddata.length() > 0)
		{
			pthread_mutex_lock(&counts_mutex);
			FCGX_PutStr(senddata.data(),senddata.length(), request.out);
			pthread_mutex_unlock(&counts_mutex);
		}
	}

finish:
	FCGX_Finish_r(&request);
  }

exit:
  return NULL;
}


int main(int argc, char *argv[])
{
   
   string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
   char *global_conf = (char *)str_global_conf.c_str();

   FCGX_Init();

   cpu_count = GetPrivateProfileInt(global_conf,"default","cpu_count");
   if (cpu_count < 1)
   {
      goto release;
   }

   id = new pthread_t[cpu_count];
   if(id == NULL)
   {
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
   {
	   delete []id;
	   id = NULL;
   }
 
   return 0;
}

