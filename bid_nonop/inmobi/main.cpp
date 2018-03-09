#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "inmobi_response.h"
#include "../../common/errorcode.h"
#include "../../common/bid_filter_type.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF	"dsp_inmobi.conf"

//uint64_t g_logid, g_logid_local;

uint64_t g_ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;

static void *doit(void *arg)
{
	int errcode = 0;
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;

	string senddata = "";
	int rc = 0;
	string requestid = "";
	timespec ts1, ts2;

	pthread_detach(pthread_self());

	RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
	if (recvdata == NULL)
	{
	//	cflog(g_logid_local, LOGERROR, "recvdata:alloc memory failed!");
		goto exit;
	}

	recvdata->length = 0;
	recvdata->buffer_length = MAX_REQUEST_LENGTH;
	recvdata->data = (char *)malloc(recvdata->buffer_length);
	if (recvdata->data == NULL)
	{
//		cflog(g_logid_local, LOGERROR, "recvdata->data: malloc memory failed!");
		goto exit;
	}

//	cflog(g_logid_local, LOGINFO, "doit:: arg:0x%x", arg);

	FCGX_InitRequest(&request, 0, 0);

	while (1)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		requestid = "";

		/* Some platforms require accept() serialization, some don't.. */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);
		
		if (rc < 0)
		{
//			cflog(g_logid_local, LOGERROR, "FCGX_Accept_r return!, rc:%d", rc);
			break;
		}

		getTime(&ts1);

		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		if (remoteaddr == NULL)
		{
//			cflog(g_logid_local, LOGERROR, "not find REMOTE_ADDR");
			continue;	
		}
//		cflog(g_logid_local, LOGINFO, "REMOTE_ADDR: %s", remoteaddr);

		char *method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		if (method == NULL)
		{
//			cflog(g_logid_local, LOGERROR, "not find REQUEST_METHOD");
			continue;
		}
//		cflog(g_logid_local, LOGINFO, "REQUEST_METHOD: %s", method);
		if (strcmp(method, "POST") != 0)
		{
//			cflog(g_logid_local, LOGERROR, "REQUEST_METHOD:%s is not POST.", method);
			continue;
		}

		char *version = FCGX_GetParam("HTTP_X_OPENRTB_VERSION", request.envp);
		if (version == NULL)
		{
//			cflog(g_logid_local, LOGERROR, "not find HTTP_X_OPENRTB_VERSION");
			continue;
		}
//		cflog(g_logid_local, LOGINFO, "HTTP_X_OPENRTB_VERSION: %s", version);

		char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
		if (contenttype == NULL)
		{
//			cflog(g_logid_local, LOGERROR, "not find CONTENT_TYPE");
			continue;
		}
//		cflog(g_logid_local, LOGINFO, "CONTENT_TYPE: %s", contenttype);
		//OpenRTB: "application/json"
		if (strcasecmp(contenttype, "application/json") &&
			strcasecmp(contenttype, "application/json; charset=utf-8"))//inmobi has "charset=utf-8", but firefox has "UTF-8"
		{
//			cflog(g_logid_local, LOGERROR, "Wrong Content-Type: %s", contenttype);
			continue;
		}

		uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
		if (contentlength == 0)
		{
//			cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
			continue;
		}
//		cflog(g_logid_local, LOGINFO, "CONTENT_LENGTH: %d", contentlength);
		if (contentlength >= recvdata->buffer_length)
		{
//			cflog(g_logid_local, LOGERROR, "CONTENT_LENGTH is too big!!!!!!! contentlength:%d! realloc recvdata.", contentlength);

			recvdata->buffer_length = contentlength+1;
			recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length);
			if (recvdata->data == NULL)
			{
//				cflog(g_logid_local, LOGERROR, "recvdata->data: realloc memory failed!");
				goto exit;
			}
		}

		recvdata->length = contentlength;

		if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
		{
//			cflog(g_logid_local, LOGERROR, "FCGX_GetStr failed!");
			continue;
		}
		recvdata->data[recvdata->length] = 0;

		errcode = getBidResponse(g_ctx, index, NULL, recvdata, senddata, requestid);
		if (errcode != E_BAD_REQUEST)
		{
			pthread_mutex_lock(&counts_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&counts_mutex);
		}
	
finish:
		FCGX_Finish_r(&request);

//		record_cost(g_logid_local, LOGDEBUG, requestid.c_str(), "send-recv", ts1);
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
	uint8_t i = 0;
	void *status = NULL;
	bool is_textdata = false;;

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
