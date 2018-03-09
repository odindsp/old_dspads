#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include <iostream>
#include <stdlib.h>
#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include "../../common/setlog.h"
#include "../../common/getlocation.h"
#include "../../common/server_log.h"
#include "../../common/confoperation.h"
#include "letv_response.h"
using namespace std;

#define LETV_CONF 			"dsp_letv.conf"
#define LETV_MAKE_CONF		"make_letv.txt"
#define ADVCATTABLE			"advcat_letv.txt"

uint64_t g_logid_response, g_logid_local, g_logid;

uint64_t cpu_count = 0;
pthread_t *id = NULL;
uint64_t g_ctx = 0;
uint64_t geodb = 0;

bool fullreqrecord = false;
map<string, vector<uint32_t> > adv_cat_table_adxi;
MD5_CTX hash_ctx;

map<string, uint16_t> letv_make_table;

bool run_flag = true;

void sigroutine(int dunno)
{
	switch (dunno) {
	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		break;
	default:
		break;
	}
}

static void *doit(void *arg)
{
	int errcode = E_SUCCESS;
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	char *remoteaddr, *contenttype, *version;
	uint32_t contentlength = 0;
	int rc;
	FLOW_BID_INFO flow_bid(ADX_LETV);

	FCGX_InitRequest(&request, 0, 0);

	while (run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		RECVDATA *recvdata = NULL;
		string senddata = "";

		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if (rc < 0)
			goto fcgi_finish;

		flow_bid.reset();
		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			recvdata = (RECVDATA *)malloc(sizeof(RECVDATA));
			if (recvdata == NULL)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata:alloc memory failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				flow_bid.set_err(E_MALLOC);
				goto fcgi_finish;
			}

			recvdata->length = contentlength;
			recvdata->data = (char *)calloc(1, contentlength + 1);
			if (recvdata->data == NULL)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata->data:alloc memory failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				flow_bid.set_err(E_MALLOC);
				goto fcgi_finish;
			}

			if (FCGX_GetStr(recvdata->data, contentlength, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				recvdata->data = NULL;
				recvdata = NULL;
				writeLog(g_logid_local, LOGERROR, "FCGX_GetStr failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				flow_bid.set_err(E_REQUEST_HTTP_BODY);
				goto fcgi_finish;
			}

			errcode = letvResponse(index, g_ctx, recvdata, senddata, flow_bid);

			if (errcode == E_REDIS_CONNECT_INVALID)
			{
				syslog(LOG_INFO, "redis connect invalid");
				kill(getpid(), SIGTERM);
			}

			if (errcode != E_BAD_REQUEST)
			{
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}
		}
		else
		{
			char *requestparam = FCGX_GetParam("QUERY_STRING", request.envp);
			if (requestparam != NULL && strstr(requestparam, "pxene_debug=1") != NULL)
			{
				string data;
				get_server_content(g_ctx, index, data);
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(data.data(), data.length(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}

			writeLog(g_logid_local, LOGERROR, "Not POST.");
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
		}

	fcgi_finish:
		g_ser_log.send_detail_log(index, flow_bid.to_string());
		FCGX_Finish_r(&request);
	}

exit:
	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
	cout << "doit " << (int)index << " exit" << endl;
	return NULL;
}

int main(int argc, char *argv[])
{
	int err = E_SUCCESS;
	bool is_textdata = false;
	bool is_print_time = false;
	vector<uint32_t> v_cat;
	map<string, uint16_t>::iterator it;
	map<string, vector<uint32_t> >::iterator it_cat;
	string letv_private_conf = "", letv_global_conf = "", letv_make = "";
	char *global_conf = NULL, *private_conf = NULL;
	char *ipbpath = NULL;
	FCGX_Init();
	vector<pthread_t> thread_id;

	letv_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	global_conf = (char *)letv_global_conf.c_str();
	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		FAIL_SHOW;
		err = E_INVALID_CPU_COUNT;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
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

	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		FAIL_SHOW;
		err = E_FILE_IPB;
		run_flag = false;
		goto exit;
	}

	letv_private_conf = string(GLOBAL_PATH) + string(LETV_CONF);
	private_conf = (char *)letv_private_conf.c_str();

	letv_make = string(GLOBAL_PATH) + string(LETV_MAKE_CONF);

	if (!init_category_table_t(letv_make, callback_insert_pair_make, (void *)&letv_make_table, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_MAKE;
		run_flag = false;
		goto exit;
	}
#ifdef DEBUG
	for (it = letv_make_table.begin(); it != letv_make_table.end(); ++it)
	{
		cout<< "dump make: " << it->first << " -> " << it->second << endl;
	}
#endif
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), 
		callback_insert_pair_string, (void *)&adv_cat_table_adxi, false))
	{
		FAIL_SHOW;
		va_cout("init adxcat table failed!");
		err = E_PROFILE_INIT_ADVCAT;
		run_flag = false;
		goto exit;
	}
#ifdef DEBUG
	for (it_cat = adv_cat_table_adxi.begin(); it_cat != adv_cat_table_adxi.end(); ++it_cat)
	{
		string val;
		cout << it_cat->first <<" : " ;
		for (int i = 0; i < it_cat->second.size(); i++)
		{
			cout << it_cat->second[i] << ",";
		}

		cout << endl;
	}

	v_cat = adv_cat_table_adxi["17"];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] <<endl;
#endif
	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	g_logid_response = openLog(private_conf, "logresponse", is_textdata, &run_flag, thread_id, is_print_time);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);

	g_logid = openLog(private_conf, "log", is_textdata, &run_flag, thread_id, is_print_time);
	
	if (g_logid == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	if (g_logid_local == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID_LOC;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	if (g_logid_response == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID_RESP;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");

	signal(SIGINT, sigroutine);//signal
	signal(SIGTERM, sigroutine);
	/*signal(SIGALRM, sigroutine);

	err = set_timer_day(g_logid_local);
	if (err != 0)
	{
	goto exit;
	}*/

	MD5_Init(&hash_ctx);

	err = bid_filter_initialize(g_logid_local, ADX_LETV, cpu_count, &run_flag, private_conf, thread_id, &g_ctx);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		writeLog(g_logid_local, LOGERROR, "bid_filter_initialize failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	err = g_ser_log.init(cpu_count, ADX_LETV, g_logid_local, private_conf, &run_flag);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		run_flag = false;
		goto exit;
	}

	for (uint8_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}

exit:
	g_ser_log.uninit();

	if (geodb != 0)
	{
		closeIPB(geodb);
	}

	if (g_logid_local != 0)
		closeLog(g_logid_local);

	if (g_logid_response != 0)
		closeLog(g_logid_response);

	if (g_logid != 0)
		closeLog(g_logid);

	if (ipbpath != NULL)
	{
		free(ipbpath);
	}

	if (g_ctx != 0)
		bid_filter_uninitialize(g_ctx);

	if (err == E_SUCCESS)
	{
		for (uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}

	if (id)
	{
		free(id);
		id = NULL;
	}

	cout << "main exit, errcode:0x" << hex << err << endl;
	return err;
}
