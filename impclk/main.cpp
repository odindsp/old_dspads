#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <queue>
#include "impclk.h"
using namespace std;


#define 	SLOGCONF			"dsp_settle.conf"
#define 	LOGCONF				"dsp_impclk.conf"
#define		PRICECONF			"dsp_price.conf"

uint64_t g_logid_local = 0, g_logid = 0;
pthread_t *id = NULL;
uint8_t cpu_count = 0;
uint64_t g_ctx = 0;
pthread_mutex_t g_mutex_id, g_mutex_ur, g_mutex_group;
vector<string> cmds_id;

vector<GROUP_FREQUENCY_INFO> v_group_frequency;
vector<USER_INFO> v_user_frequency;

bool run_flag = true;
DATATRANSFER *g_datatransfer;
DATATRANSFER *g_datatransfer_s;

char *master_group_ip = NULL, *slave_group_ip = NULL, *master_ur_ip = NULL, *slave_ur_ip = NULL;
uint16_t  master_group_port = 0, slave_group_port = 0, master_ur_port = 0, slave_ur_port = 0;

void* frequency_process(void *arg)
{
	uint64_t logid = *(uint64_t *)arg;

	redisContext *ctx_gr_r = NULL;
	redisContext *ctx_gr_w = NULL;

	ctx_gr_r = redisConnect(slave_group_ip, slave_group_port);
	if (ctx_gr_r == NULL)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process read connnect is NULL");
	}
	else if (ctx_gr_r->err)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process read failed,err:%s", ctx_gr_r->errstr);

		redisFree(ctx_gr_r);
		ctx_gr_r = NULL;
	}

	ctx_gr_w = redisConnect(master_group_ip, master_group_port);
	if (ctx_gr_w == NULL)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process write connnect is NULL");
	}
	else if (ctx_gr_w->err)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process write failed,err:%s", ctx_gr_w->errstr);

		redisFree(ctx_gr_w);
		ctx_gr_w = NULL;
	}

	do 
	{
	//	usleep(PIPETIME);
		pthread_mutex_lock(&g_mutex_group);
		long fre_size = v_group_frequency.size();
		if(fre_size > 0)
		{
			vector<GROUP_FREQUENCY_INFO> v_group_frequency_temp;
			v_group_frequency_temp.swap(v_group_frequency);
			pthread_mutex_unlock(&g_mutex_group);

			struct timespec ts1, ts2;
			long lasttime = 0;
			getTime(&ts1);
			for(int i = 0; i < fre_size; ++i)
			{
				int errcode = E_SUCCESS;
				if (ctx_gr_r == NULL)
				{
				   ctx_gr_r = redisConnect(slave_group_ip, slave_group_port);
					if (ctx_gr_r == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process read connnect is NULL");
					}
					else if (ctx_gr_r->err)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process read failed,err:%s", ctx_gr_r->errstr);

						redisFree(ctx_gr_r);
						ctx_gr_r = NULL;
					}
				}

				if (ctx_gr_w == NULL)
				{
				   ctx_gr_w = redisConnect(master_group_ip, master_group_port);
					if (ctx_gr_w == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process write connnect is NULL");
					}
					else if (ctx_gr_w->err)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process write failed,err:%s", ctx_gr_w->errstr);

						redisFree(ctx_gr_w);
						ctx_gr_w = NULL;
					}
				}

				if (ctx_gr_r != NULL && ctx_gr_w != NULL)
				{
				    errcode = check_group_frequency_capping(ctx_gr_r, ctx_gr_w, &v_group_frequency_temp[i]);
				    if (errcode == E_REDIS_CONNECT_R_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping read redisContext is invaild");
						redisFree(ctx_gr_r);
						ctx_gr_r = NULL;
					}
					else if (errcode == E_REDIS_CONNECT_W_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping write redisContext is invaild");
						redisFree(ctx_gr_w);
						ctx_gr_w = NULL;
					}
				}
			}

			getTime(&ts2);
			lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;	
			cflog(logid, LOGDEBUG, "frequency_process size:%ld,spent time:%ld", fre_size, lasttime);
		}
		else
		{
			pthread_mutex_unlock(&g_mutex_group);
			usleep(PIPETIME);
		}

	} while (run_flag);

exit:
	if (ctx_gr_r != NULL)
		redisFree(ctx_gr_r);
	if (ctx_gr_w != NULL)
		redisFree(ctx_gr_w);
	cout<<"frequency_process exit"<<endl;
}

void* read_write_ur(void *arg)
{
	uint64_t logid = *(uint64_t *)arg;

	redisContext *ctx_ur_r = NULL;
	redisContext *ctx_ur_w = NULL;

	ctx_ur_r = redisConnect(slave_ur_ip, slave_ur_port);
	if (ctx_ur_r == NULL)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur read connnect is NULL");
	}
	else if (ctx_ur_r->err)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur read failed,err:%s", ctx_ur_r->errstr);

		redisFree(ctx_ur_r);
		ctx_ur_r = NULL;
	}

	ctx_ur_w = redisConnect(master_ur_ip, master_ur_port);
	if (ctx_ur_w == NULL)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur write connnect is NULL");
	}
	else if (ctx_ur_w->err)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur write failed,err:%s", ctx_ur_w->errstr);

		redisFree(ctx_ur_w);
		ctx_ur_w = NULL;
	}

	do 
	{
	//	usleep(PIPETIME);
		pthread_mutex_lock(&g_mutex_ur);
		long fre_size = v_user_frequency.size();
		if(fre_size > 0)
		{
			vector<USER_INFO> v_user_frequency_temp;
			v_user_frequency_temp.swap(v_user_frequency);
			pthread_mutex_unlock(&g_mutex_ur);

			struct timespec ts1, ts2;
			long lasttime = 0;
			getTime(&ts1);
			for(int i = 0; i < fre_size; ++i)
			{
				int errcode = E_SUCCESS;
				if (ctx_ur_r == NULL)
				{
				   ctx_ur_r = redisConnect(slave_ur_ip, slave_ur_port);
					if (ctx_ur_r == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur read connnect is NULL");
					}
					else if (ctx_ur_r->err)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur read failed,err:%s", ctx_ur_r->errstr);

						redisFree(ctx_ur_r);
						ctx_ur_r = NULL;
					}
				}

				if (ctx_ur_w == NULL)
				{
				   ctx_ur_w = redisConnect(master_ur_ip, master_ur_port);
					if (ctx_ur_w == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur write connnect is NULL");
					}
					else if (ctx_ur_w->err)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur write failed,err:%s", ctx_ur_w->errstr);

						redisFree(ctx_ur_w);
						ctx_ur_w = NULL;
					}
				}

				if (ctx_ur_r != NULL && ctx_ur_w != NULL)
				{
				    errcode = UserRestrictionCount(ctx_ur_r, ctx_ur_w, logid, &v_user_frequency_temp[i]);
				    if (errcode == E_REDIS_CONNECT_R_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping read redisContext is invaild");
						redisFree(ctx_ur_r);
						ctx_ur_r = NULL;
					}
					else if (errcode == E_REDIS_CONNECT_W_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping write redisContext is invaild");
						redisFree(ctx_ur_w);
						ctx_ur_w = NULL;
					}
				}
			}

			getTime(&ts2);
			lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;	
			cflog(logid, LOGDEBUG, "read_write_ur size:%ld,spent time:%ld", fre_size, lasttime);
		}
		else
		{
			pthread_mutex_unlock(&g_mutex_ur);
			usleep(PIPETIME);
		}

	} while (run_flag);

exit:
	if (ctx_ur_r != NULL)
		redisFree(ctx_ur_r);
	if (ctx_ur_w != NULL)
		redisFree(ctx_ur_w);
	cout<<"read_write_ur exit"<<endl;
}

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	int rc = 0;
	FCGX_Request request;
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)g_ctx;

	char *remoteaddr = NULL, *requestparam = NULL;
	string senddata;

	DATATRANSFER *datatransfer = &g_datatransfer[index/2];
	DATATRANSFER *datatransfer_s = g_datatransfer_s;

	if (pctx == NULL)
	{
		writeLog(g_logid_local, LOGERROR, "pctx is null");
		goto exit;
	}
	senddata = pctx->messagedata;

	FCGX_InitRequest(&request, 0, 0);

	while(run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;		
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		int errcode = E_SUCCESS;

		/* Accept the request message */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		writeLog(g_logid_local, LOGINFO,"rc:%d", rc);
		if(rc < 0)
		{
			writeLog(g_logid_local, LOGERROR, "FCGX_Accept_r failed");
			goto fcgi_finish;
		}

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
	//	requesturi = FCGX_GetParam("REQUEST_URI", request.envp);
		requestparam = FCGX_GetParam("QUERY_STRING", request.envp);

		if((remoteaddr != NULL) && (requestparam != NULL))
		{
			writeLog(g_logid_local, LOGDEBUG, string(remoteaddr) + " " + string(requestparam));
			errcode = parseRequest(request, g_ctx, index, datatransfer, datatransfer_s, remoteaddr, requestparam);
			if (errcode == E_SUCCESS)
			{
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}
			else
				writeLog(g_logid_local, LOGERROR, "parseRequest invalid,err:0x%x", errcode);
		}
		else
			writeLog(g_logid_local, LOGERROR, "remoteaddr  or requestparam is NULL!");

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

int main(int argc, char *argv[])
{
	pthread_t thread_id, thread_ur, thread_group;
	char *impclk_ip = NULL, *settle_ip = NULL, *master_filter_id_ip = NULL;
	uint16_t impclk_port = 0, settle_port = 0, master_filter_id_port = 0;

	int errcode = E_SUCCESS, err_id = E_UNKNOWN, err_ur = E_UNKNOWN, err_group = E_UNKNOWN;
	int send_count =0;
	string impclk_conf = "", global_conf = "";

	PIPELINE_INFO wr_id;

	global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	impclk_conf = string(GLOBAL_PATH) + string(LOGCONF);

	char *sz_global_conf = (char *)global_conf.c_str();
	char *sz_impclk_conf = (char *)impclk_conf.c_str();

	vector<pthread_t> t_thread;

	//sinal for exit
	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	FCGX_Init();

	err_id = pthread_mutex_init(&g_mutex_id, 0);
	if (err_id != E_SUCCESS)
	{
		errcode = E_MUTEX_INIT;
		goto exit;
	}

	err_ur = pthread_mutex_init(&g_mutex_ur, 0);
	if (err_ur != E_SUCCESS)
	{
		errcode = E_MUTEX_INIT;
		goto exit;
	}

	err_group = pthread_mutex_init(&g_mutex_group, 0);
	if (err_group != E_SUCCESS)
	{
		errcode = E_MUTEX_INIT;
		goto exit;
	}

	cpu_count = GetPrivateProfileInt(sz_global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		errcode = E_INVALID_CPU_COUNT;
		goto exit;
	}

	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (id == NULL)
	{
		errcode = E_MALLOC;
		va_cout("id malloc failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
send_count = cpu_count/2;	
	g_datatransfer = new DATATRANSFER[send_count + 1];
	if (g_datatransfer == NULL)
	{
		errcode = E_MALLOC;
		goto exit;
	}
	g_datatransfer_s = new DATATRANSFER;
	if (g_datatransfer_s == NULL)
	{
		errcode = E_MALLOC;
		goto exit;
	}
	impclk_ip = GetPrivateProfileString(sz_global_conf, "logserver", "imp_clk_ip");
	impclk_port = GetPrivateProfileInt(sz_global_conf, "logserver", "imp_clk_port");

	settle_ip = GetPrivateProfileString(sz_global_conf, "logserver", "settle_ip");
	settle_port = GetPrivateProfileInt(sz_global_conf, "logserver", "settle_port");

	if ((impclk_ip == NULL) || (impclk_port == 0) || (settle_ip == NULL) || (settle_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
	
	g_logid_local = openLog(sz_impclk_conf, "locallog", true, &run_flag, t_thread);
	g_logid = openLog(sz_impclk_conf, "log", true, &run_flag, t_thread);
	if((g_logid_local == 0) || (g_logid == 0))
	{
		errcode = E_INVALID_PARAM_LOGID;
		va_cout("openlog failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	master_ur_ip = GetPrivateProfileString(sz_impclk_conf, (char *)"redis", (char *)"master_ur_ip");
	master_ur_port = GetPrivateProfileInt(sz_impclk_conf, (char *)"redis", (char *)"master_ur_port");

	slave_ur_ip = GetPrivateProfileString(sz_impclk_conf, (char *)"redis", (char *)"slave_ur_ip");
	slave_ur_port = GetPrivateProfileInt(sz_impclk_conf, (char *)"redis", (char *)"slave_ur_port");
	
	master_filter_id_ip = GetPrivateProfileString(sz_impclk_conf, (char *)"redis", (char *)"master_filter_id_ip");
	master_filter_id_port = GetPrivateProfileInt(sz_impclk_conf, (char *)"redis", (char *)"master_filter_id_port");

	master_group_ip = GetPrivateProfileString(sz_impclk_conf, (char *)"redis", (char *)"master_gr_ip");
	master_group_port = GetPrivateProfileInt(sz_impclk_conf, (char *)"redis", (char *)"master_gr_port");

	slave_group_ip = GetPrivateProfileString(sz_impclk_conf, (char *)"redis", (char *)"slave_gr_ip");
	slave_group_port = GetPrivateProfileInt(sz_impclk_conf, (char *)"redis", (char *)"slave_gr_port");

	if((master_ur_ip == NULL) || (slave_ur_ip == NULL) || (master_filter_id_ip == NULL) || (master_group_ip == NULL) || (slave_group_ip == NULL) 
		||  (master_ur_port == 0) || (slave_ur_port == 0) || (master_filter_id_port == 0) || (master_group_port == 0) || (slave_group_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		writeLog(g_logid_local, LOGERROR, "read profile file failed,err:0x%x", errcode);
		goto exit;
	}


	errcode = global_initialize(g_logid_local, cpu_count, &run_flag, sz_impclk_conf, t_thread, &g_ctx);
	if ((errcode != E_SUCCESS))
	{
		writeLog(g_logid_local, LOGERROR, "global_initialize failed,err:0x%x", errcode);
		goto exit;
	}

	//Redis Operation
	init_pipeinfo(g_logid_local,"redis_write_id", master_filter_id_ip, master_filter_id_port, &cmds_id, &g_mutex_id, &run_flag, &wr_id);
	errcode = pthread_create(&thread_id, NULL, threadpipeline, &wr_id);
	if (errcode != E_SUCCESS)
		goto exit;
	t_thread.push_back(thread_id);

	errcode = pthread_create(&thread_ur, NULL, read_write_ur, &g_logid_local);
	if (errcode != E_SUCCESS)
		goto exit;
	t_thread.push_back(thread_ur);

	errcode = pthread_create(&thread_group, NULL, frequency_process, &g_logid_local);
	if (errcode != E_SUCCESS)
		goto exit;
	t_thread.push_back(thread_group);

	for(int i = 0 ; i <=(cpu_count/2); i++)
	{
		connect_log_server(g_logid_local, impclk_ip, impclk_port, &run_flag, &g_datatransfer[i]);
		t_thread.push_back(g_datatransfer[i].threadlog);
	}

	connect_log_server(g_logid_local, settle_ip, settle_port, &run_flag, g_datatransfer_s);
	t_thread.push_back(g_datatransfer_s->threadlog);
	


	for (uint64_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0 ; i < t_thread.size(); ++i)
	{
		pthread_join(t_thread[i], NULL);
	}

exit:
	if (err_id == E_SUCCESS)
		pthread_mutex_destroy(&g_mutex_id);

	if (err_ur == E_SUCCESS)
		pthread_mutex_destroy(&g_mutex_ur);

	if (err_group == E_SUCCESS)
		pthread_mutex_destroy(&g_mutex_group);

	
	IFFREENULL(impclk_ip);
	IFFREENULL(settle_ip);
	IFFREENULL(master_ur_ip);
	IFFREENULL(slave_ur_ip);
	IFFREENULL(master_filter_id_ip);
	IFFREENULL(master_group_ip);
    IFFREENULL(slave_group_ip);

	if(g_logid_local)
		closeLog(g_logid_local);

	if(g_logid)
		closeLog(g_logid);

	if (g_ctx != 0)
	{
		int errcode = global_uninitialize(g_ctx);
		g_ctx = 0;
	}

	if (g_datatransfer)
	{
		for (uint8_t i = 0; i <= cpu_count /2; ++i)
		{
			disconnect_log_server(&g_datatransfer[i]);
		}

		delete []g_datatransfer;
		g_datatransfer = NULL;
	}

	if (g_datatransfer_s)
	{
		disconnect_log_server(g_datatransfer_s);
		delete g_datatransfer_s;
		g_datatransfer_s = NULL;
	}

	if (errcode == E_SUCCESS)
	{
		for(uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}

	IFFREENULL(id);

	cout << "odin impclk exit,errcode:0x" << hex << errcode << endl;
	return errcode;
}
