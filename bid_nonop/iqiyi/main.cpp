#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "iqiyi_adapter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"
#include "../../common/errorcode.h"
using namespace std;

#define PRIVATE_CONF  "dsp_iqiyi.conf"
#define APPCATTABLE   "appcat_iqiyi.txt"

using namespace std;
uint64_t ctx = 0;
pthread_t *tid = NULL;
uint64_t g_logid, g_logid_local, g_logid_response;

char *g_server_ip;
uint32_t g_server_port;
int cpu_count;
bool fullreqrecord = false;
bool run_flag = true;

static void *doit(void *arg)
{
	//uint64_t ctx = (uint64_t)arg;
	uint8_t index = (uint64_t)arg;
	//pthread_t threadlog;
	FCGX_Request request;
	string senddata = "";
	int errorcode = 0;
	int rc = 0;
//	char *remoteaddr, *contenttype, *version;
//	uint32_t contentlength = 0;

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
			/*char *version = FCGX_GetParam("HTTP_X_AMAXRTB_VERSION", request.envp);
			if (version)
			{
				va_cout("find! x-iqiyirtb-version: %s", version);
			}
			else
			{
				va_cout("not find! x-openrtb-version! ");
				writeLog(g_logid_local, LOGINFO, "not find! x-openrtb-version! ");
				continue;
			}*/

			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: %s", contenttype);

			//OpenRTB: "application/json"
			if (strcasecmp(contenttype, "application/x-protobuf"))
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
				recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length + 1);
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
			if(fullreqrecord)
				writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);

			errorcode = iqiyi_adapter(recvdata, senddata);

			pthread_mutex_lock(&counts_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&counts_mutex);
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

void sigroutine(int dunno)
{ /* 信号处理例程，其中dunno将会得到信号的值 */

	switch (dunno) {
	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		{
			syslog(LOG_INFO, "Get a signal -- %d", dunno);
			run_flag = false;
			break;
		}
	default:
		cout<<"Get a unknown -- "<< dunno << endl;
		break;
	}

	return;
}

int main(int argc, char *argv[])
{
	int err = E_SUCCESS;
	bool is_textdata = false;
	vector<pthread_t> thread_id;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if(cpu_count == 0)
	{
		err = E_INVALID_PROFILE;
		goto exit;
	}

	tid = new pthread_t[cpu_count];

	FCGX_Init();
	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, true);
	if(g_logid_local == 0)
	{
		err = E_INVALID_PARAM_LOGID;
		goto exit;
	}

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(private_conf, "log", is_textdata, &run_flag, thread_id, true);
	if(g_logid == 0)
	{
		err = E_INVALID_PARAM_LOGID;
		goto exit;
	}

	is_textdata = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");
	va_cout("logresponse is_textdata  %d", is_textdata);
	g_logid_response = openLog(private_conf, "logresponse", is_textdata, &run_flag, thread_id, true);
	if(g_logid_response == 0)
	{
		err = E_INVALID_PARAM_LOGID;
		goto exit;
	}

	g_server_ip = GetPrivateProfileString(private_conf, "logserver", "bid_ip");
	if(g_server_ip == NULL)
	{
		err = E_INVALID_PROFILE;
		goto exit;
	}

	g_server_port = GetPrivateProfileInt(private_conf, "logserver", "bid_port");
	if(g_server_port == 0)
	{
		err = E_INVALID_PROFILE;
		goto exit;
	}

	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");

	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);


	for (uint8_t i = 0; i < cpu_count; i++)
		pthread_create(&tid[i], NULL, doit, (void *)i);

	for (uint8_t i = 0 ; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}

exit:

	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

	if(g_logid_response)
		closeLog(g_logid_response);

	if (g_server_ip)
		free(g_server_ip);

	if (err == E_SUCCESS)
	{
		for(uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(tid[i], SIGKILL);
		}
	}
	if (tid)
	{
		delete tid;
		tid = NULL;
	}
	return 0;
}
