#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "iflytek_response.h"
#include "../../common/errorcode.h"
#include "../../common/bid_filter.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF	"dsp_iflytek.conf"
#define APPCATTABLE		"appcat_iflytek.txt"
#define ADVCATTABLE		"advcat_iflytek.txt"
#define DEVMAKETABLE  "make_iflytek.txt"


uint64_t g_logid_local, g_logid_local_traffic, g_logid, g_logid_response;

map<string, vector<uint32_t> > app_cat_table_adx2com;
map<string, vector<uint32_t> > adv_cat_table_adx2com;
//map<uint32_t, vector<string> > adv_cat_table_com2adx;
map<string, uint16_t> dev_make_table;

uint64_t g_ctx = 0;
uint8_t cpu_count = 0;
uint64_t *numcount = NULL;
pthread_t *id = NULL;
bool fullreqrecord = false;
uint8_t ferr_level = FERR_LEVEL_CREATIVE;
bool is_print_time = false;
MD5_CTX hash_ctx;
uint64_t geodb = 0;

bool run_flag = true;


static void *doit(void *arg)
{
	int errcode = 0;
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	char *remoteaddr = NULL;
	char *method = NULL;
	char *version = NULL;
	char *contenttype = NULL;
	uint32_t contentlength = 0;
	int rc = 0;
	string requestid = "";
	string response_short = "";
	timespec ts1, ts2;
	FLOW_BID_INFO flow_bid(ADX_IFLYTEK);

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

		recvdata->buffer_length = 0;
		goto exit;
	}

	cflog(g_logid_local, LOGINFO, "doit:: arg:0x%x", arg);

	FCGX_InitRequest(&request, 0, 0);

	while (run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		requestid = response_short = "";

		errcode = E_BAD_REQUEST;
		string senddata;

		/* Some platforms require accept() serialization, some don't.. */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if (rc < 0)
		{
			cflog(g_logid_local, LOGERROR, "FCGX_Accept_r return!, rc:%d", rc);
			break;
		}

		getTime(&ts1);

		flow_bid.reset();
		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		if (remoteaddr == NULL)
		{
			cflog(g_logid_local, LOGERROR, "not find REMOTE_ADDR");
			flow_bid.set_err(E_REQUEST_HTTP_REMOTE_ADDR);
			goto loopend;
		}
		//		cflog(g_logid_local, LOGINFO, "REMOTE_ADDR: %s", remoteaddr);

		method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		if (method == NULL)
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find REQUEST_METHOD", remoteaddr);
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
			goto loopend;
		}
		//		cflog(g_logid_local, LOGINFO, "REQUEST_METHOD: %s", method);
		if (strcmp(method, "POST") != 0)
		{
			char *requestparam = FCGX_GetParam("QUERY_STRING", request.envp);
			if (requestparam != NULL && strstr(requestparam, "pxene_debug=1") != NULL)
			{
				string data;
				get_server_content(g_ctx, index, data);
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(data.data(), data.length(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, REQUEST_METHOD: %s is not POST.", remoteaddr, method);
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
			goto loopend;
		}

		version = FCGX_GetParam("HTTP_X_OPENRTB_VERSION", request.envp);
		if (version == NULL)
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find HTTP_X_OPENRTB_VERSION", remoteaddr);
			flow_bid.set_err(E_REQUEST_HTTP_OPENRTB_VERSION);
			goto loopend;
		}
		if (strcasecmp(version, "2.3.1"))
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, HTTP_X_OPENRTB_VERSION: %s, Wrong x-openrtb-version", remoteaddr, version);
			flow_bid.set_err(E_REQUEST_HTTP_OPENRTB_VERSION);
			goto loopend;
		}

		contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
		if (contenttype == NULL)
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find CONTENT_TYPE", remoteaddr);
			flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
			goto loopend;
		}
		//		cflog(g_logid_local, LOGINFO, "CONTENT_TYPE: %s", contenttype);

		if (strcasecmp(contenttype, "application/json") &&//OpenRTB: "application/json"
			strcasecmp(contenttype, "application/json; charset=utf-8"))//inmobi has "charset=utf-8", but firefox has "UTF-8"
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, CONTENT_TYPE: %s, Wrong Content-Type", remoteaddr, contenttype);
			flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
			goto loopend;
		}

		contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
		if (contentlength == 0)
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, not find CONTENT_LENGTH or is 0", remoteaddr);
			flow_bid.set_err(E_REQUEST_HTTP_CONTENT_LEN);
			goto loopend;
		}
		//		cflog(g_logid_local, LOGINFO, "CONTENT_LENGTH: %d", contentlength);

		if (contentlength >= recvdata->buffer_length)
		{
			recvdata->buffer_length = contentlength + 1;
			recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length);
			if (recvdata->data == NULL)
			{
				cflog(g_logid_local, LOGERROR, "remoteaddr: %s, recvdata->data: realloc memory failed!", remoteaddr);
				flow_bid.set_err(E_MALLOC);
				goto exit;
			}
		}

		recvdata->length = contentlength;

		if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
		{
			cflog(g_logid_local, LOGERROR, "remoteaddr: %s, FCGX_GetStr failed!", remoteaddr);
			flow_bid.set_err(E_REQUEST_HTTP_BODY);
			goto loopend;
		}

		recvdata->data[recvdata->length] = 0;

		errcode = get_bid_response(g_ctx, index, recvdata, senddata, requestid, response_short, flow_bid);

		if (errcode == E_INVALID_PARAM)
			goto loopend;
		else if (errcode == E_BAD_REQUEST)
		{
			cflog(g_logid_local_traffic, LOGMAX, "E_BAD_REQUEST: %s", recvdata->data);

			goto loopend;
		}

		if (fullreqrecord)
		{
			cflog(g_logid_local_traffic, LOGMAX, "requestid: %s, in: %s, out: %s", requestid.c_str(), recvdata->data, senddata.c_str());
		}

		if (errcode == E_SUCCESS)
		{
			cflog(g_logid, LOGMAX, "%s", recvdata->data);

			cflog(g_logid_response, LOGMAX, "%s", response_short.c_str());

			if ((!fullreqrecord) && (++numcount[index] % 1000 == 0))
			{
				cflog(g_logid_local_traffic, LOGMAX, "requestid: %s, in: %s, out: %s", requestid.c_str(), recvdata->data, senddata.c_str());
				numcount[index] = 0;
			}
		}
		else if (errcode == E_REDIS_CONNECT_INVALID)
		{
			syslog(LOG_INFO, "redis connect invalid");
			kill(getpid(), SIGTERM);
		}

		pthread_mutex_lock(&counts_mutex);
		FCGX_PutStr(senddata.data(), senddata.size(), request.out);
		pthread_mutex_unlock(&counts_mutex);

	loopend:
		g_ser_log.send_detail_log(index, flow_bid.to_string());
		FCGX_Finish_r(&request);

		//	record_cost(g_logid_local, is_print_time, requestid.c_str(), string(string("recv-send, err:") + hexToString(errcode)).c_str(), ts1);    
		getTime(&ts2);
		cflog(g_logid_local, LOGDEBUG, "%s,%s,time:%d", requestid.c_str(), string(string("recv-send, err:") + hexToString(errcode)).c_str(),
			((ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000));
	}

exit:

	if (recvdata != NULL)
	{
		if (recvdata->data != NULL)
			free(recvdata->data);
		free(recvdata);
	}

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
	cout << "doit " << (int)index << " exit" << endl;
	return NULL;
}

void sigroutine(int dunno)
{ /* 信号处理例程，其中dunno将会得到信号的值 */

	switch (dunno) {

	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);

		va_cout("Get a signal -- SIGINT, ctx:%d, cpu_count:%d", g_ctx, cpu_count);
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
	int errcode = E_SUCCESS;
	void *status = NULL;
	bool is_textdata = false;
	map<string, vector<uint32_t> >::iterator it;
	map<uint32_t, vector<string> >::iterator it_adv;
	map<string, uint16_t>::iterator it_make;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();
	char *ipbpath = NULL;
	vector<pthread_t> thread_id;

	cpu_count = GetPrivateProfileInt((char *)str_global_conf.c_str(), "default", "cpu_count");

	va_cout("cpu_count:%d", cpu_count);

	if (cpu_count < 1)
	{
		FAIL_SHOW;
		errcode = E_INVALID_CPU_COUNT;
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
	if (id == NULL)
	{
		FAIL_SHOW;
		errcode = E_MALLOC;
		run_flag = false;
		goto exit;
	}

	numcount = (uint64_t *)calloc(cpu_count, sizeof(uint64_t));
	if (numcount == NULL)
	{
		FAIL_SHOW;
		errcode = E_MALLOC;
		run_flag = false;
		goto exit;
	}

	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);


	FCGX_Init();

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	ferr_level = GetPrivateProfileInt(private_conf, "locallog", "ferr_level");
	va_cout("locallog ferr_level: %d", ferr_level);

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_local == 0)
	{
		FAIL_SHOW;
		va_cout("openLog failed! name:%s, section:%s", private_conf, "locallog");
		errcode = E_INVALID_PARAM_LOGID_LOC;
		run_flag = false;
		goto exit;
	}

	is_textdata = GetPrivateProfileInt(private_conf, "localtraffic", "is_textdata");
	va_cout("localtraffic is_textdata  %d", is_textdata);
	g_logid_local_traffic = openLog(private_conf, "localtraffic", is_textdata, &run_flag, thread_id);
	if (g_logid_local_traffic == 0)
	{
		cflog(g_logid_local, LOGERROR, "openLog failed! name:%s, section:%s", private_conf, "localtraffic");

		g_logid_local_traffic = g_logid_local;//如果配置文件不支持g_logid_local_traffic，则使用g_logid_local替代

	}

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(private_conf, "log", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid == 0)
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "openLog failed! name:%s, section:%s", private_conf, "log");
		errcode = E_INVALID_PARAM_LOGID;
		run_flag = false;
		goto exit;
	}

	is_textdata = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");
	va_cout("logresponse is_textdata  %d", is_textdata);
	g_logid_response = openLog(private_conf, "logresponse", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_response == 0)
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "openLog failed! name:%s, section:%s", private_conf, "logresponse");
		errcode = E_INVALID_PARAM_LOGID_RESP;
		run_flag = false;
		goto exit;
	}

	/* signal(SIGALRM, sigroutine);
   errcode = set_timer_day(g_logid_local);
   if (errcode != 0)
   {
   goto exit;
   }*/

	fullreqrecord = GetPrivateProfileInt(private_conf, "log", "fullreqrecord");
	va_cout("log fullreqrecord  %d", fullreqrecord);

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(),
		callback_insert_pair_string_hex, (void *)&app_cat_table_adx2com, false))
	{
		FAIL_SHOW;
		errcode = E_PROFILE_INIT_APPCAT;
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		run_flag = false;
		goto exit;
	}

	for (it = app_cat_table_adx2com.begin(); it != app_cat_table_adx2com.end(); ++it)
	{
		string val;
		for (int i = 0; i < it->second.size(); i++)
		{
			val += " " + hexToString(it->second[i]);
		}

		va_cout("dump app in: %s -> %s", it->first.c_str(), val.c_str());
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		callback_insert_pair_string_hex, (void *)&adv_cat_table_adx2com, false))
	{
		FAIL_SHOW;
		errcode = E_PROFILE_INIT_ADVCAT;
		cflog(g_logid_local, LOGERROR, "init adv_cat_table_adx2com table failed!");
		run_flag = false;
		goto exit;
	}

	for (it = adv_cat_table_adx2com.begin(); it != adv_cat_table_adx2com.end(); ++it)
	{
		string val;
		for (int i = 0; i < it->second.size(); i++)
		{
			val += " " + hexToString(it->second[i]);
		}

		va_cout("dump adv in: %s -> %s", it->first.c_str(), val.c_str());
	}

	/*if (!init_category_table_t(ADVCATTABLE, callback_insert_pair_hex_string, (void *)&adv_cat_table_com2adx))
	{
	cflog(g_logid_local, LOGERROR, "init adv_cat_table_com2adx table failed!");
	goto exit;
	}

	for (it_adv = adv_cat_table_com2adx.begin(); it_adv != adv_cat_table_com2adx.end(); ++it_adv)
	{
	string val;
	for (int i = 0; i < it_adv->second.size(); i++)
	{
	val += " " + it_adv->second[i];
	}

	va_cout("dump adv out: %s -> 0x%x", it->first.c_str(), val.c_str());
	}*/

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
	errcode = bid_filter_initialize(g_logid_local, ADX_IFLYTEK, cpu_count, &run_flag, private_conf, thread_id, &g_ctx);
	if (errcode != E_SUCCESS)
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "bid_filter_initialize failed! errcode:0x%x", errcode);
		run_flag = false;
		goto exit;
	}

	cflog(g_logid_local, LOGINFO, "bid_filter_initialize success! ctx:0x%x", g_ctx);

	errcode = g_ser_log.init(cpu_count, ADX_IFLYTEK, g_logid_local, private_conf, &run_flag);
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
	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid_local_traffic && g_logid_local != g_logid_local_traffic)
		closeLog(g_logid_local_traffic);

	if (g_logid)
		closeLog(g_logid);

	if (g_logid_response)
		closeLog(g_logid_response);

	if (geodb != 0)
	{
		closeIPB(geodb);
		geodb = 0;
	}

	if (g_ctx != 0)
	{
		int errcode = bid_filter_uninitialize(g_ctx);
		cout << "main: bid_filter_uninitialize, errcode:0x" << dec << errcode << endl;
		g_ctx = 0;
	}
	if (numcount)
	{
		free(numcount);
		numcount = NULL;
	}

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
