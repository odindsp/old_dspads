#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include "adwalker_response.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF "dsp_adwalker.conf"

uint64_t g_logid, g_logid_local;
bool fullreqrecord = false;

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
		cflog(g_logid_local, LOGERROR, "recvdata:alloc memory failed!");
		goto exit;
	}

	recvdata->length = 0;
	recvdata->buffer_length = MAX_REQUEST_LENGTH;
	recvdata->data = (char *)malloc(recvdata->buffer_length);
	if (recvdata->data == NULL)
	{
		cflog(g_logid_local, LOGERROR, "recvdata->data: malloc memory failed!");
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
			cflog(g_logid_local, LOGERROR, "FCGX_Accept_r return!, rc:%d", rc);
			break;
		}

		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		va_cout("remoteaddr: %s", remoteaddr);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			char *version = FCGX_GetParam("HTTP_X_AMAXRTB_VERSION", request.envp);
			if (version)
			{
				va_cout("find! x-amaxrtb-version: %s", version);
			}
			else
			{
				va_cout("not find! x-amaxrtb-version! ");
				writeLog(g_logid_local, LOGINFO, "not find! x-amaxrtb-version! ");
				continue;
			}

			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: %s", contenttype);

			//OpenRTB: "application/json"
			if (strcasecmp(contenttype, "application/json") &&
				strcasecmp(contenttype, "application/json; charset=utf-8"))//Adwalker has "charset=utf-8", but firefox has "UTF-8"
			{
				va_cout("Wrong contenttype: %s", contenttype);
				writeLog(g_logid_local, LOGINFO, "Content-Type error.");
				continue;
			}

			uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			va_cout("contentlength: %d", contentlength);
			if (contentlength == 0)
			{
				cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
				continue;
			}
			//		cflog(g_logid_local, LOGINFO, "CONTENT_LENGTH: %d", contentlength);
			if (contentlength >= recvdata->buffer_length)
			{
				cflog(g_logid_local, LOGERROR, "CONTENT_LENGTH is too big!!!!!!! contentlength:%d! realloc recvdata.", contentlength);

				recvdata->buffer_length = contentlength;
				recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length);
				if (recvdata->data == NULL)
				{
					cflog(g_logid_local, LOGERROR, "recvdata->data: realloc memory failed!");
					goto exit;
				}
			}

			recvdata->length = contentlength;

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				writeLog(g_logid_local, LOGINFO, "fread fail");
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
			writeLog(g_logid_local, LOGINFO, "Not POST.");
		}

		FCGX_Finish_r(&request);
	}
exit:

	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
}


int main(int argc, char *argv[])
{
	pthread_t *tid = NULL;
	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();
	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();

	int cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if(cpu_count == 0)
		return 1;

	tid = new pthread_t[cpu_count];
	int err = 0;
	bool is_textdata = false;
	
	FCGX_Init();
	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);
	g_logid_local = openLog(private_conf, "locallog", is_textdata);
	if(g_logid_local == 0)
		goto exit;

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(private_conf, "log", is_textdata);
	if(g_logid == 0)
		goto exit;

	fullreqrecord = GetPrivateProfileInt(private_conf, "log", "fullreqrecord");

	tid[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; i++)
		pthread_create(&tid[i], NULL, doit, (void *)i);

	doit(0);
exit:

	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

	if (tid)
	{
		delete tid;
		tid = NULL;
	}

	return 0;
}
