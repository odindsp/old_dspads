#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <signal.h>
#include "zplay_response.h"
using namespace std;

#define ZPLAY_CONF		"dsp_zplay.conf"
#define APPCATTABLE		"appcat_zplay.txt"

uint64_t g_logid_local, g_logid, g_logid_response;

uint64_t cpu_count = 0;
pthread_t *id = NULL;
uint64_t g_ctx = 0;

bool is_print_time = false;

static void *doit(void *arg)
{
	int errcode = E_SUCCESS;
	uint8_t index = (uint64_t)arg;
	pthread_t threadlog;
	FCGX_Request request;
	char *remoteaddr = NULL, *contenttype = NULL, *version = NULL;
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
			goto fcgi_finish;

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			if (strcmp("application/x-protobuf", FCGX_GetParam("CONTENT_TYPE", request.envp)) == 0)
			{
				contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
				recvdata = (RECVDATA *)malloc(sizeof(RECVDATA));
				if (recvdata == NULL)
				{
					writeLog(g_logid_local, LOGERROR, "recvdata:alloc memory failed!");
					goto fcgi_finish;
				}

				recvdata->length = contentlength;
				recvdata->data = (char *)calloc(1, contentlength + 1);
				if (recvdata->data == NULL)
				{
					writeLog(g_logid_local, LOGERROR,"recvdata->data:alloc memory failed!");
					goto fcgi_finish;
				}

				if (FCGX_GetStr(recvdata->data, contentlength, request.in) == 0)
				{
					free(recvdata->data);
					free(recvdata);
					recvdata->data = NULL;
					recvdata = NULL;
					writeLog(g_logid_local, LOGERROR, "FCGX_GetStr fail");
					goto fcgi_finish;
				}
				errcode = zplayResponse(index, recvdata, senddata);

				if (errcode != E_BAD_REQUEST)
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(senddata.data(), senddata.size(), request.out);
					pthread_mutex_unlock(&context_mutex);
				}
			}
			else
				writeLog(g_logid_local, LOGERROR, "Not application/x-protobuf");
		}
		else
			writeLog(g_logid_local, LOGERROR, "Not POST.");

fcgi_finish:

		FCGX_Finish_r(&request);
	}
}

int main(int argc, char *argv[])
{
	int errcode = E_SUCCESS;
	bool is_textdata = false;
	map<string, vector<uint32_t> >::iterator it;
	string zplay_private_conf = "",zplay_appcat = "", zplay_global_conf = "";
	char *private_conf = NULL, *global_conf = NULL;
	FCGX_Init();
	zplay_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	global_conf = (char *)zplay_global_conf.c_str();

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	
	zplay_private_conf = string(GLOBAL_PATH) + string(ZPLAY_CONF);

	private_conf = (char *)zplay_private_conf.c_str();

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	g_logid_response = openLog(private_conf, "logresponse", is_textdata, is_print_time);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, is_print_time);
	g_logid = openLog(private_conf, "log", is_textdata, is_print_time);
	if ((g_logid_response == 0) || (g_logid_local == 0) || (g_logid == 0))
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}

	id[0] = pthread_self();
	for (uint64_t i = 1; i < cpu_count; ++i)
		pthread_create(&id[i], NULL, doit, (void *)i);
	doit(0);

exit:
	if (g_logid_local != 0)
		closeLog(g_logid_local);
	
	if (g_logid != 0)
		closeLog(g_logid);

	return errcode;
}
