#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include "sohu_response.h"
using namespace std;

#define SOHU_CONF		"dsp_sohu.conf"
#define APPCATTABLE		"appcat_sohu.txt"

uint64_t cpu_count = 0;
pthread_t *id = NULL;

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	uint32_t contentlength = 0;
	string senddata = "";
	int rc;

	pthread_detach(pthread_self());

	FCGX_InitRequest(&request, 0, 0);

	for (;;)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		RECVDATA *recvdata = NULL;

		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if (rc < 0)
			break;

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			if (strcmp("application/x-protobuf", FCGX_GetParam("CONTENT_TYPE", request.envp)) == 0)
			{
				contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
				recvdata = (RECVDATA *)malloc(sizeof(RECVDATA));
				if (recvdata == NULL)
				{
					continue;
				}

				recvdata->length = contentlength;
				recvdata->data = (char *)calloc(1, contentlength + 1);
				if (recvdata->data == NULL)
				{
					continue;
				}

				if (FCGX_GetStr(recvdata->data, contentlength, request.in) == 0)
				{
					free(recvdata->data);
					free(recvdata);
					recvdata->data = NULL;
					recvdata = NULL;
					continue;
				}
				sohuResponse(index, recvdata, senddata);
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}
		}

		FCGX_Finish_r(&request);
	}
}

int main(int argc, char *argv[])
{
	string sohu_global_conf = "";

	FCGX_Init();
	
	sohu_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	
	cpu_count = GetPrivateProfileInt((char *)sohu_global_conf.c_str(), "default", "cpu_count");
	
	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));

	id[0] = pthread_self();
	for (uint64_t i = 1; i < cpu_count; ++i)
		pthread_create(&id[i], NULL, doit, (void *)i);
	doit(0);
	
	if (id)
	{
	  free(id);
	  id = NULL;
	}

	return 0;
}
