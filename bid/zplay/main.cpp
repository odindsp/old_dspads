#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <signal.h>
#include <syslog.h>
#include "zplay_response.h"
using namespace std;

#define ZPLAY_CONF		"dsp_zplay.conf"
#define APPCATTABLE		"appcat_zplay.txt"
#define DEVMAKETABLE  "make_zplay.txt"

uint64_t g_logid_local, g_logid, g_logid_response;
uint64_t cpu_count = 0;
pthread_t *id = NULL;
uint64_t g_ctx = 0;
MD5_CTX hash_ctx;

map<string, vector<uint32_t> > zplay_app_cat_table;
map<string, uint16_t> dev_make_table;
bool is_print_time = false;
uint64_t geodb = 0;

bool run_flag = true;
int *numcount = NULL;

void sigroutine(int dunno)
{
	switch (dunno) {
	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		break;
	case SIGALRM:
		syslog(LOG_INFO, "receive SIGALRM signal");
		if (g_ctx != 0)
		{
			syslog(LOG_INFO, "main set timer before");
			bid_time_clear(g_ctx);
			syslog(LOG_INFO, "main bid_time_clear");
			set_timer_day(g_logid_local);
			syslog(LOG_INFO, "main set timer after");
		}
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
	char *remoteaddr = NULL, *contenttype = NULL, *version = NULL;
	uint32_t contentlength = 0;
	int rc;
	FLOW_BID_INFO flow_bid(ADX_ZPLAY);

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
			if (strcmp("application/x-protobuf", FCGX_GetParam("CONTENT_TYPE", request.envp)) == 0)
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
				errcode = zplayResponse(index, g_ctx, recvdata, senddata, flow_bid);

				if (errcode != E_BAD_REQUEST)
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(senddata.data(), senddata.size(), request.out);
					pthread_mutex_unlock(&context_mutex);
				}
			}
			else
			{
				writeLog(g_logid_local, LOGERROR, "Not application/x-protobuf");
				flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
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
	int errcode = E_SUCCESS;
	bool is_textdata = false, is_textdata_log = false, is_textdata_response = false;
	map<string, vector<uint32_t> >::iterator it;
	string zplay_private_conf = "", zplay_appcat = "", zplay_global_conf = "";
	char *private_conf = NULL, *global_conf = NULL;
	char *ipbpath = NULL;
	map<string, uint16_t>::iterator it_make;
	FCGX_Init();
	zplay_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	global_conf = (char *)zplay_global_conf.c_str();
	vector<pthread_t> thread_id;

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_CPU_COUNT;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}
	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if (ipbpath == NULL)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PROFILE_IPB;
		run_flag = false;
		goto exit;
	}

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		FAIL_SHOW;
		errcode = E_FILE_IPB;
		run_flag = false;
		goto exit;
	}

	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	numcount = (int *)calloc(cpu_count, sizeof(int));

	zplay_private_conf = string(GLOBAL_PATH) + string(ZPLAY_CONF);

	private_conf = (char *)zplay_private_conf.c_str();

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	is_textdata_log = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	is_textdata_response = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	g_logid_response = openLog(private_conf, "logresponse", is_textdata_response, &run_flag, thread_id, is_print_time);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);
	g_logid = openLog(private_conf, "log", is_textdata_log, &run_flag, thread_id, is_print_time);
	if (g_logid == 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PARAM_LOGID;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}
	if (g_logid_local == 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PARAM_LOGID_LOC;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}
	if (g_logid_response == 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PARAM_LOGID_RESP;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	signal(SIGINT, sigroutine);//signal
	signal(SIGTERM, sigroutine);


	zplay_appcat = string(GLOBAL_PATH) + string(APPCATTABLE);

	if (!init_category_table_t(zplay_appcat, callback_insert_pair_string_hex, (void *)&zplay_app_cat_table, false))
	{
		FAIL_SHOW;
		errcode = E_PROFILE_INIT_APPCAT;
		writeLog(g_logid_local, LOGERROR, "init_category_table_t failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	for (it = zplay_app_cat_table.begin(); it != zplay_app_cat_table.end(); ++it)
	{
		string val;
		for (int i = 0; i < it->second.size(); i++)
		{
			val += " " + hexToString(it->second[i]);
		}

		va_cout("dump app in: %s -> %s", it->first.c_str(), val.c_str());
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(),
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		FAIL_SHOW;
		errcode = E_PROFILE_INIT_MAKE;
		va_cout("init make table failed!!");
		run_flag = false;
		goto exit;
	}

	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout << "dump make: " << it_make->first << " -> " << it_make->second << endl;
	}


	MD5_Init(&hash_ctx);

	errcode = bid_filter_initialize(g_logid_local, ADX_ZPLAY, cpu_count, &run_flag, private_conf, thread_id, &g_ctx);
	if (errcode != E_SUCCESS)
	{
		FAIL_SHOW;
		writeLog(g_logid_local, LOGERROR, "bid_filter_initialize failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	errcode = g_ser_log.init(cpu_count, ADX_ZPLAY, g_logid_local, private_conf, &run_flag);
	if (errcode != E_SUCCESS)
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

	if (g_logid_local != 0)
		closeLog(g_logid_local);

	if (g_logid != 0)
		closeLog(g_logid);

	if (geodb != 0)
	{
		closeIPB(geodb);
		geodb = 0;
	}
	if (numcount)
	{
		free(numcount);
		numcount = NULL;
	}

	if (g_ctx != 0)
		bid_filter_uninitialize(g_ctx);

	if (errcode == E_SUCCESS)
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

	cout << "main exit, errcode:0x" << hex << errcode << endl;

	return errcode;
}
