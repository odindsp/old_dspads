#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "homeTV_response.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"
#include "../../common/errorcode.h"

#define PRIVATE_CONF  "dsp_hometv.conf"

using namespace std;
pthread_t *tid = NULL;
uint64_t g_logid_local;

int cpu_count;
bool fullreqrecord = false;
bool is_print_time = false;

bool run_flag = true;

static void *doit(void *arg)
{
	FCGX_Request request;
	string senddata;
	int errcode = E_SUCCESS;
	char *remoteaddr = NULL, *requestparam = NULL;
	int rc = 0;

	FCGX_InitRequest(&request, 0, 0);
	while (run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;

		/* Some platforms require accept() serialization, some don't.. */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);
		if (rc < 0)
		{
			cflog(g_logid_local, LOGERROR, "FCGX_Accept_r return!, rc:%d", rc);
			break;
		}

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		if (strcmp("GET", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			requestparam = FCGX_GetParam("QUERY_STRING", request.envp);

			if((remoteaddr != NULL) && (requestparam != NULL))
			{
				writeLog(g_logid_local, LOGDEBUG, string(remoteaddr) + " " + string(requestparam));
				errcode = getBidResponse(remoteaddr, requestparam, senddata);
				if (errcode == E_SUCCESS)
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(senddata.data(), senddata.size(), request.out);
					pthread_mutex_unlock(&context_mutex);
				}
			}
			else
				writeLog(g_logid_local, LOGERROR, "remoteaddr  or requestparam is NULL!");
		}
		else
		{
			va_cout("Not GET.");
			writeLog(g_logid_local, LOGINFO, "Not GET.");
		}

nextLoop:
		FCGX_Finish_r(&request);
	}
exit:

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
}

int main(int argc, char *argv[])
{
	int err = E_SUCCESS;
	bool is_textdata = false;
	vector<uint32_t> v_cat;
	map<string, vector<uint32_t> >::iterator it;
	//map<uint32_t, vector<uint32_t> >::iterator it_i;
	map<string, uint16_t>::iterator it_make;
	char *ipbpath = NULL;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();
	
	vector<pthread_t> thread_id;

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if(cpu_count == 0)
	{
		FAIL_SHOW
		err = E_INVALID_CPU_COUNT;
		goto exit;
	}

	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if(ipbpath == NULL)
	{
		FAIL_SHOW
        err = E_INVALID_PROFILE;
		goto exit;
	}

	tid = new pthread_t[cpu_count];

	FCGX_Init();

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time  %d", is_print_time);

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);

	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);
	if(g_logid_local == 0)
	{
		FAIL_SHOW
		err = E_INVALID_PARAM_LOGID;
		goto exit;
	}

	tid = new pthread_t[cpu_count];

	tid[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; ++i)
	{
		pthread_create(&tid[i], NULL, doit, (void *)i);
	}
	doit(0);
exit:

	if (g_logid_local)
		closeLog(g_logid_local);

	if (err == E_SUCCESS)
	{
		for(uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(tid[i], SIGKILL);
		}
	}
	if (tid)
	{
		delete [] tid;
		tid = NULL;
	}

	cout<<"main exit, errcode:0x"<< hex << err << endl;

	return err;
}
