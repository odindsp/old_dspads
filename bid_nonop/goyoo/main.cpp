#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include "../../common/confoperation.h"
#include "goyoo_response.h"

using namespace std;

static void *doit(void *arg)
{
	pthread_detach(pthread_self());
	uint8_t index = (uint64_t)arg;
	pthread_t threadlog;
	FCGX_Request request;
	string senddata = "";
	int errorcode = 0;
	int rc = 0;

	RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
	if (recvdata == NULL)
	{
		goto exit;
	}

	recvdata->length = 0;
	recvdata->buffer_length = MAX_REQUEST_LENGTH;
	recvdata->data = (char *)malloc(recvdata->buffer_length);
	if (recvdata->data == NULL)
	{
		goto exit;
	}

	FCGX_InitRequest(&request, 0, 0);

	while (1)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		/* Some platforms require accept() serialization, some don't.. */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);
		if (rc < 0)
		{
			break;
		}

		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		va_cout("remoteaddr: %s", remoteaddr);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: application/x-protobuf");

			if (strcasecmp(contenttype, "application/x-protobuf"))
			{
				va_cout("Wrong contenttype: %s", contenttype);
				continue;
			}

			uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			va_cout("contentlength: %d", contentlength);
			if (contentlength == 0)
			{
				continue;
			}

			if (contentlength >= recvdata->buffer_length)
			{
				recvdata->buffer_length = contentlength;
				recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length);
				if (recvdata->data == NULL)
				{
					goto exit;
				}
			}

			recvdata->length = contentlength;

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				continue;
			}
			recvdata->data[recvdata->length] = 0;
	
			errorcode = getBidResponse(recvdata, senddata);
			if(errorcode ==  0)
			{
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

		}
		else
		{
			va_cout("Not POST.");
		}

		FCGX_Finish_r(&request);
	}

exit:
	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);
}


int main(int argc, char *argv[])
{
	pthread_t *tid = NULL;
	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	int cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if(cpu_count == 0)
		return 1;

	tid = new pthread_t[cpu_count];
	int err = 0;
	bool is_textdata = false;
	
	FCGX_Init();

	tid[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; i++)
		pthread_create(&tid[i], NULL, doit, (void *)i);

	doit(0);

exit:
	if (tid)
	{
		delete tid;
		tid = NULL;
	}

	return 0;
}
