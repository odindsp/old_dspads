/* DSP Settle */
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "settle.h"
#include "../common/confoperation.h"
#include "../common/type.h"

using namespace std;

#define LOGCONF				"dsp_settle.conf"

uint64_t g_logid_local = 0, g_logid = 0;

pthread_t *id = NULL;
uint8_t cpu_count = 0;
uint64_t g_ctx = 0;

pthread_mutex_t g_mutex_id;
vector<string> cmds;

bool run_flag = true;
DATATRANSFER *g_datatransfer;

static void* doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	int rc = 0;
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)g_ctx;
	string senddata;
	char *remoteaddr = NULL, *requestparam = NULL;
//	DATATRANSFER *datatransfer = &g_datatransfer[index];
	if (pctx == NULL)
	{
		writeLog(g_logid_local, LOGERROR, "pctx is null");
		goto exit;
	}
	senddata = pctx->messagedata;

	FCGX_InitRequest(&request, 0, 0);
 
	while(run_flag)
	{
			static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
			static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
			int errcode = E_SUCCESS;

			pthread_mutex_lock(&accept_mutex);
			rc = FCGX_Accept_r(&request);
			pthread_mutex_unlock(&accept_mutex);

			if (rc < 0)
			{
				writeLog(g_logid_local, LOGERROR, "FCGX_Accept_r failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				goto fcgi_finish;
			}

			remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
			requestparam = FCGX_GetParam("QUERY_STRING", request.envp);

			if((remoteaddr != NULL) && (requestparam != NULL))
			{
				//Parse Request and send the .gif info to web 

				writeLog(g_logid_local, LOGDEBUG, string(remoteaddr) + " " + string(requestparam));
				errcode = parseRequest(g_ctx, index, g_datatransfer, remoteaddr, requestparam);

				if (errcode == E_SUCCESS)
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(senddata.data(), senddata.size(), request.out);//Send info to web
					pthread_mutex_unlock(&context_mutex);
				}
				else
					writeLog(g_logid_local, LOGERROR, "request invalid");
			}
			else
			{
				writeLog(g_logid_local, LOGERROR, "remoteaddr or requestparam is NULL!");
				goto fcgi_finish;
			}

fcgi_finish:
			FCGX_Finish_r(&request);
	}

exit:
	cout<<"doit exit"<<endl;
	return NULL;
}


void sigroutine(int dunno)
{
	switch (dunno)
	{
	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		run_flag = false;
		break;
	default:
		break;
	
	}
}

int main(int argc, char *agrv[])
{
	pthread_t thread_t, thread_t_pipeline;
	string private_conf = "", global_conf = "";
	int errcode = E_SUCCESS, err_id = E_UNKNOWN;
	char *settle_ip = NULL;
	uint16_t settle_port = 0;
	char *master_filter_id_ip = NULL;
	uint16_t master_filter_id_port = 0;
	PIPELINE_INFO wr_id;
	/* FCGX muti-thread init */
	FCGX_Init();

	global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	private_conf = string(GLOBAL_PATH) + string(LOGCONF);

	char *sz_global_conf = (char *)global_conf.c_str();
	char *sz_private_conf = (char *)private_conf.c_str();
	
	vector<pthread_t> t_thread;

	err_id = pthread_mutex_init(&g_mutex_id, 0);
	if (err_id != E_SUCCESS)
	{
		errcode = E_MUTEX_INIT;
		goto exit;
	}

	cpu_count = GetPrivateProfileInt(sz_global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		errcode = E_INVALID_CPU_COUNT;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	
	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (id == NULL)
	{
		errcode = E_MALLOC;
		va_cout("id malloc failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	g_datatransfer = new DATATRANSFER;
	if (g_datatransfer == NULL)
	{
		errcode = E_MALLOC;
		goto exit;
	}

	g_logid_local = openLog(sz_private_conf, "locallog", true, &run_flag, t_thread);
	g_logid = openLog(sz_private_conf, "log", true, &run_flag, t_thread);
	if((g_logid_local == 0) || (g_logid == 0))
	{
		errcode = E_INVALID_PARAM_LOGID;
		va_cout("openlog failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	master_filter_id_ip = GetPrivateProfileString(sz_private_conf, (char *)"redis", (char *)"master_filter_id_ip");
	master_filter_id_port = GetPrivateProfileInt(sz_private_conf, (char *)"redis", (char *)"master_filter_id_port");

	settle_ip = GetPrivateProfileString(sz_global_conf, "logserver", "settle_ip");
	settle_port = GetPrivateProfileInt(sz_global_conf, "logserver", "settle_port");


	if ((master_filter_id_ip == NULL) || (master_filter_id_port == 0) ||
		(settle_ip == NULL) || (settle_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		writeLog(g_logid_local, LOGERROR, "read profile file failed,err:0x%x", errcode);
		goto exit;
	}


	errcode = global_initialize(g_logid_local, cpu_count, &run_flag, sz_private_conf, t_thread, &g_ctx);
	if(errcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "global_initialize failed,err:0x%x", errcode);
		goto exit;
	}

	/* Signal for exit */
	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);
	
	/* Write redis within pipeline */
	init_pipeinfo(g_logid_local,"redis_write_id", master_filter_id_ip, master_filter_id_port, &cmds, &g_mutex_id, &run_flag, &wr_id);
	errcode = pthread_create(&thread_t_pipeline, NULL, threadpipeline, &wr_id);
	if (errcode != E_SUCCESS)
		goto exit;
	t_thread.push_back(thread_t_pipeline);

	connect_log_server(g_logid_local, settle_ip, settle_port, &run_flag, g_datatransfer);
	t_thread.push_back(g_datatransfer->threadlog);
    
	for(uint64_t i = 0;i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void*)i);
	}
	
	for (uint8_t i = 0 ; i < t_thread.size(); ++i)
	{
	   pthread_join(t_thread[i], NULL);
	}

exit:
	if (err_id == E_SUCCESS)
		pthread_mutex_destroy(&g_mutex_id);

	
	IFFREENULL(settle_ip);
	IFFREENULL(master_filter_id_ip);

	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

	if (g_ctx != 0)
	{
		int errcode = global_uninitialize(g_ctx);
		va_cout("global_uninitialize(ctx:0x%x), errcode:0x%x", g_ctx, errcode);
		g_ctx = 0;
	}

	if (g_datatransfer)
	{
		disconnect_log_server(g_datatransfer);
		delete g_datatransfer;
		g_datatransfer = NULL;
	}


	if (errcode == E_SUCCESS)
	{
		for(uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}
	IFFREENULL(id);

	cout << "odin settle exit,errcode:0x" << hex << errcode << endl;

	return errcode;
}

