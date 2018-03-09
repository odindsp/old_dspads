#include <sys/stat.h>
#include <iostream>
#include <stdlib.h>
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <map>
#include <vector>
#include "../../common/confoperation.h"
#include "google_response.h"
#include <signal.h>
#include <syslog.h>
using namespace std;

#define PRIVATE_CONF  "dsp_google.conf"

uint64_t g_logid_local;
uint8_t cpu_count = 0;
pthread_t *id = NULL;

bool run_flag = true;

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	string senddata = "";
	int errorcode = E_SUCCESS;
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
	while (run_flag)
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
			uint32_t contentlength;

			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: %s", contenttype);

			//OpenRTB: "application/json"
			if (strcasecmp(contenttype, "application/octet-stream") && strcasecmp(contenttype, "application/x-protobuf"))
			{
				va_cout("Wrong contenttype: %s", contenttype);
				writeLog(g_logid_local, LOGINFO, "Content-Type error.");
				goto nextLoop;
			}

			contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			va_cout("contentlength: %d", contentlength);
			if (contentlength == 0)
			{
				cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
				goto nextLoop;
			}
			//		cflog(g_logid_local, LOGINFO, "CONTENT_LENGTH: %d", contentlength);
			if (contentlength >= recvdata->buffer_length)
			{
				cflog(g_logid_local, LOGERROR, "CONTENT_LENGTH is too big!!!!!!! contentlength:%d! realloc recvdata.", contentlength);

				recvdata->buffer_length = contentlength + 1;
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
				writeLog(g_logid_local, LOGINFO, "fread fail");
				goto nextLoop;
			}
			recvdata->data[recvdata->length] = 0;

			errorcode = getBidResponse(recvdata, senddata);
			if(errorcode !=  E_BAD_REQUEST)
			{
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

			if(errorcode == E_REDIS_CONNECT_INVALID)
			{
				syslog(LOG_INFO, "redis connect invalid");
				kill(getpid(), SIGTERM);
			}

		}
		else
		{
			va_cout("Not POST.");
			writeLog(g_logid_local, LOGINFO, "Not POST.");
		}
nextLoop:
		FCGX_Finish_r(&request);
	}
exit:

	if (recvdata != NULL)
	{
		if (recvdata->data != NULL)
			free(recvdata->data);
		free(recvdata);
	}

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
	cout<<"doit "<<(int)index<<" exit"<<endl;
}

void sigroutine(int dunno)
{
	switch (dunno)
	{
	case SIGINT:
	case SIGTERM:
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		break;
	default:
		cout<<"Get a unknown -- "<< dunno << endl;
		break;
	}

	return;
}

int main(int argc, char *argv[])
{
	int err = E_SUCCESS;
	string logdir;
	bool is_textdata = false;
	bool is_print_time = false;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();
	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();
	vector<pthread_t> thread_id;

	FCGX_Init();

	cpu_count = GetPrivateProfileInt(global_conf,"default","cpu_count");
	if (cpu_count < 1)
	{
		FAIL_SHOW
			err = E_INVALID_CPU_COUNT;
		va_cout("cpu_count error!");
		goto release;
	}

	id = new pthread_t[cpu_count];
	if(id == NULL)
	{
		va_cout("allocate pthread_t error");
		va_cout("cpu_count=%d",cpu_count);
		FAIL_SHOW
			err = E_MALLOC;
		goto release;
	}
	va_cout("cpu_count=%d",cpu_count);

	signal(SIGINT, sigroutine);
	signal(SIGTERM,sigroutine);

	logdir = "locallog";
	is_textdata = GetPrivateProfileInt(private_conf,(char *)(logdir.c_str()), "is_textdata");
	va_cout("loglocal is_textdata  %d", is_textdata);
	is_print_time = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	g_logid_local = openLog(private_conf, (char *)(logdir.c_str()), is_textdata, &run_flag, thread_id, is_print_time);	
	if(g_logid_local == 0)
	{
		FAIL_SHOW
			err = E_INVALID_PARAM_LOGID;
		va_cout("open local log error");
		goto release;
	}

	for (uint8_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0 ; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}
release:
	if(g_logid_local != 0)
		closeLog(g_logid_local);

	google::protobuf::ShutdownProtobufLibrary();

	if (err == E_SUCCESS)
	{
		for(uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}

	if(id != NULL)
	{
		delete []id;
		id = NULL;
	}
	cout<<"main exit, errcode:0x"<< hex << err << endl;
	return err;
}

