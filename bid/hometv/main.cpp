#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "homeTV_response.h"
#include "../../common/bid_filter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"
#include "../../common/errorcode.h"

#define PRIVATE_CONF  "dsp_hometv.conf"
#define DEVMAKETABLE  "make_hometv.txt"

using namespace std;
uint64_t ctx = 0;
uint64_t geodb = 0;
int *numcount = NULL;
pthread_t *tid = NULL;
uint64_t g_logid, g_logid_local, g_logid_response;
map<string, uint16_t> dev_make_table;


int cpu_count;
bool fullreqrecord = false;
bool is_print_time = false;
MD5_CTX hash_ctx;

bool run_flag = true;


static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	int errcode = E_SUCCESS;
	char *remoteaddr = NULL, *requestparam = NULL;
	int rc = 0;
	FLOW_BID_INFO flow_bid(ADX_HOMETV);

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

		string senddata;
		flow_bid.reset();
		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		if (strcmp("GET", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			requestparam = FCGX_GetParam("QUERY_STRING", request.envp);

			if (requestparam != NULL && strstr(requestparam, "pxene_debug=1") != NULL)
			{
				string data;
				get_server_content(ctx, index, data);
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(data.data(), data.length(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}

			if ((remoteaddr != NULL) && (requestparam != NULL))
			{
				writeLog(g_logid_local, LOGDEBUG, string(remoteaddr) + " " + string(requestparam));
				errcode = getBidResponse(ctx, index, remoteaddr, requestparam, senddata, flow_bid);
				if (errcode != E_BAD_REQUEST)
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(senddata.data(), senddata.size(), request.out);
					pthread_mutex_unlock(&context_mutex);
				}
				else if (errcode == E_REDIS_CONNECT_INVALID)
				{
					syslog(LOG_INFO, "redis connect invalid");
					kill(getpid(), SIGTERM);
				}
			}
			else
			{
				writeLog(g_logid_local, LOGERROR, "remoteaddr  or requestparam is NULL!");
				flow_bid.set_err(E_REQUEST_HTTP_REMOTE_ADDR);
			}
		}
		else
		{
			va_cout("Not GET.");
			writeLog(g_logid_local, LOGINFO, "Not GET.");
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
		}

	nextLoop:
		g_ser_log.send_detail_log(index, flow_bid.to_string());
		FCGX_Finish_r(&request);
	}
exit:

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
	cout << "doit " << (int)index << " exit" << endl;
}

void sigroutine(int dunno)
{ /* 信号处理例程，其中dunno将会得到信号的值 */

	switch (dunno) {

	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		break;
	default:
		cout << "Get a unknown -- " << dunno << endl;
		break;

	}

	return;
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
	if (cpu_count == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_CPU_COUNT;
		run_flag = false;
		goto exit;
	}

	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if (ipbpath == NULL)
	{
		FAIL_SHOW;
		err = E_INVALID_PROFILE_IPB;
		run_flag = false;
		goto exit;
	}

	tid = new pthread_t[cpu_count];
	numcount = (int *)calloc(cpu_count, sizeof(int));

	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	FCGX_Init();

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		FAIL_SHOW;
		err = E_FILE_IPB;
		run_flag = false;
		goto exit;
	}

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time  %d", is_print_time);

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);

	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_local == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID_LOC;
		run_flag = false;
		goto exit;
	}

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(private_conf, "log", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID;
		run_flag = false;
		goto exit;
	}

	is_textdata = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");
	va_cout("logresponse is_textdata  %d", is_textdata);
	g_logid_response = openLog(private_conf, "logresponse", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_response == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID_RESP;
		run_flag = false;
		goto exit;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "init make table failed!");
		err = E_PROFILE_INIT_MAKE;
		run_flag = false;
		goto exit;
	}
#ifdef DEBUG
	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout<< "dump make: " << it_make->first << " -> " << it_make->second << endl;
	}

#endif

	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");

	//err = set_timer_day(g_logid_local);
	//if (err != 0)
	//{
	//	goto exit;
	//}

	MD5_Init(&hash_ctx);

	err = bid_filter_initialize(g_logid_local, ADX_HOMETV, cpu_count, &run_flag, private_conf, thread_id, &ctx);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "bid_filter_initialize failed! errcode:0x%x", err);
		run_flag = false;
		goto exit;
	}

	err = g_ser_log.init(cpu_count, ADX_HOMETV, g_logid_local, private_conf, &run_flag);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		run_flag = false;
		goto exit;
	}

	for (uint8_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&tid[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}

exit:
	g_ser_log.uninit();
	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

	if (g_logid_response)
		closeLog(g_logid_response);

	if (ctx != 0)
	{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;
	}

	if (geodb != 0)
	{
		closeIPB(geodb);
	}

	if (numcount)
	{
		free(numcount);
		numcount = NULL;
	}

	if (err == E_SUCCESS)
	{
		for (uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(tid[i], SIGKILL);
		}
	}
	if (tid)
	{
		delete tid;
		tid = NULL;
	}

	cout << "main exit, errcode:0x" << hex << err << endl;

	return err;
}
