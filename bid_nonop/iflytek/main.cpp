#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "iflytek_response.h"
#include "../../common/errorcode.h"
#include "../../common/confoperation.h"
using namespace std;

uint64_t g_ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;


static void *doit(void *arg)
{
	int errcode = 0;
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	char *remoteaddr = NULL;
	char *method = NULL;
	char *version = NULL;
	char *contenttype = NULL;
	uint32_t contentlength = 0;
	string senddata = "";
	int rc = 0;
	string requestid = "";
	string response_short = "";

	pthread_detach(pthread_self());

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
		recvdata->buffer_length = 0;
		goto exit;
	}

	FCGX_InitRequest(&request, 0, 0);

	while (1)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		requestid = response_short = "";

		errcode = E_BAD_REQUEST;

		/* Some platforms require accept() serialization, some don't.. */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);
		
		if (rc < 0)
		{
			break;
		}

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		if (remoteaddr == NULL)
		{
	//		cflog(g_logid_local, LOGERROR, "not find REMOTE_ADDR");

			goto loopend;
		}
//		cflog(g_logid_local, LOGINFO, "REMOTE_ADDR: %s", remoteaddr);

		method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		if (method == NULL)
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find REQUEST_METHOD", remoteaddr);

			goto loopend;
		}
//		cflog(g_logid_local, LOGINFO, "REQUEST_METHOD: %s", method);
		if (strcmp(method, "POST") != 0)
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, REQUEST_METHOD: %s is not POST.", remoteaddr, method);

			goto loopend;
		}

		version = FCGX_GetParam("HTTP_X_OPENRTB_VERSION", request.envp);
		if (version == NULL)
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find HTTP_X_OPENRTB_VERSION", remoteaddr);

			goto loopend;
		}
		if (strcasecmp(version, "2.3.1"))
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, HTTP_X_OPENRTB_VERSION: %s, Wrong x-openrtb-version", remoteaddr, version);

			goto loopend;
		}

		contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
		if (contenttype == NULL)
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find CONTENT_TYPE", remoteaddr);

			goto loopend;
		}
//		cflog(g_logid_local, LOGINFO, "CONTENT_TYPE: %s", contenttype);
		
		if (strcasecmp(contenttype, "application/json") &&//OpenRTB: "application/json"
			strcasecmp(contenttype, "application/json; charset=utf-8"))//inmobi has "charset=utf-8", but firefox has "UTF-8"
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, CONTENT_TYPE: %s, Wrong Content-Type", remoteaddr, contenttype);

			goto loopend;
		}

		contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
		if (contentlength == 0)
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find CONTENT_LENGTH or is 0", remoteaddr);

			goto loopend;
		}
//		cflog(g_logid_local, LOGINFO, "CONTENT_LENGTH: %d", contentlength);

		if (contentlength >= recvdata->buffer_length)
		{
			recvdata->buffer_length = contentlength + 1;
			recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length); 
			if (recvdata->data == NULL)
			{
	//			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, recvdata->data: realloc memory failed!", remoteaddr);

				goto exit;
			}
		}

		recvdata->length = contentlength;

		if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
		{
	//		cflog(g_logid_local, LOGERROR, "remoteaddr: %s, FCGX_GetStr failed!", remoteaddr);

			goto loopend;
		}

		recvdata->data[recvdata->length] = 0;

		errcode = get_bid_response(g_ctx, index, recvdata, senddata);

		if (errcode == E_SUCCESS)
		{
			pthread_mutex_lock(&counts_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&counts_mutex);
			va_cout("success!");
		}

loopend:

		FCGX_Finish_r(&request);  
	}

exit:

	if (recvdata != NULL)
	{
		if (recvdata->data != NULL)
		  free(recvdata->data);
		free(recvdata);
    }
  
}

int main(int argc, char *argv[])
{
	int errcode = 0;
	void *status = NULL;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	cpu_count = GetPrivateProfileInt((char *)str_global_conf.c_str(), "default", "cpu_count");
	
	va_cout("cpu_count:%d", cpu_count);

	if (cpu_count < 1)
	{
		va_cout("cpu_count error!");
		errcode = -1;
		goto exit;
	}

	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (id == NULL)
	{
		errcode = -1;
		goto exit;
	}

	FCGX_Init();

	id[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; i++)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	doit(0);

exit:

	if (id)
	{
		free(id);
		id = NULL;
	}
	return 0;
}
