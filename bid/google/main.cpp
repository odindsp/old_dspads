#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <syslog.h>
#include <assert.h>
#include <errno.h>
#include <map>
#include <vector>
#include <stdlib.h>
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/confoperation.h"
#include "google_response.h"

using namespace std;


#define PRIVATE_CONF  "dsp_google.conf"
#define APPCATTABLE   "appcat_google.txt"
#define ADVCATTABLE	  "advcat_google.txt"
#define DEVMAKETABLE  "make_google.txt"
#define CREATIVEATRR  "creative-attributes_google.txt"
#define GEOTOIPB      "geo_ipb.txt"

uint64_t g_logid, g_logid_local, g_logid_response;
uint64_t ctx = 0;
uint64_t geodb = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;
map<int, vector<uint32_t> > adv_cat_table_adxi;
map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
map<int, vector<uint32_t> > app_cat_table;
map<string, uint16_t> dev_make_table;
map<uint8_t, uint8_t> creative_attr_in;
map<uint8_t, vector<uint8_t> > creative_attr_out;
map<int, int> geo_ipb;
MD5_CTX hash_ctx;
int *numcount = NULL;

uint8_t ferr_level = FERR_LEVEL_CREATIVE;

bool run_flag = true;

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	int errorcode = 0;
	int rc = 0;
	FLOW_BID_INFO flow_bid(ADX_GOOGLE);

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
		string senddata;

		flow_bid.reset();
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
				flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
				goto nextLoop;
			}

			contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			va_cout("contentlength: %d", contentlength);
			if (contentlength == 0)
			{
				cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
				flow_bid.set_err(E_REQUEST_HTTP_CONTENT_LEN);
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
					flow_bid.set_err(E_MALLOC);
					goto exit;
				}
			}

			recvdata->length = contentlength;

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				writeLog(g_logid_local, LOGINFO, "fread fail");
				flow_bid.set_err(E_REQUEST_HTTP_BODY);
				goto nextLoop;
			}

			recvdata->data[recvdata->length] = 0;
			if (fullreqrecord)
				writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);

			errorcode = getBidResponse(ctx, index, recvdata, senddata, flow_bid);
			if (errorcode != E_BAD_REQUEST)
			{
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

			if (errorcode == E_REDIS_CONNECT_INVALID)
			{
				syslog(LOG_INFO, "redis connect invalid");
				kill(getpid(), SIGTERM);
			}

		}
		else
		{
			char *requestparam = FCGX_GetParam("QUERY_STRING", request.envp);
			if (requestparam != NULL && strstr(requestparam, "pxene_debug=1") != NULL)
			{
				string data;
				get_server_content(ctx, index, data);
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(data.data(), data.length(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

			va_cout("Not POST.");
			writeLog(g_logid_local, LOGINFO, "Not POST.");
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
		}

	nextLoop:
		g_ser_log.send_detail_log(0, flow_bid.to_string());
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
	cout << "doit " << (int)index << " exit" << endl;
}

void sigroutine(int dunno)
{
	switch (dunno)
	{
	case SIGINT:
	case SIGTERM:
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		/*if(dunno == SIGINT)
		syslog(LOG_INFO,"recv sigint");
		else
		syslog(LOG_INFO,"recv sigterm");

		va_cout("Get a signal -- %d, ctx:%d, cpu_count:%d", dunno,ctx, cpu_count);
		if (geodb != 0)
		{
		closeIPB(geodb);
		geodb = 0;
		}
		if (ctx != 0)
		{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;

		for (int i = 0; i < cpu_count; i++)
		{
		va_cout("pthread_kill[%d]:%d...", i, id[i]);
		pthread_kill(id[i], SIGKILL);
		va_cout("pthread_kill[%d]:%d over", i, id[i]);
		}
		}*/
		break;
	case SIGALRM:
		syslog(LOG_INFO, "receive SIGALRM signal");
		if (ctx != 0)
		{
			syslog(LOG_INFO, "main set timer before");
			bid_time_clear(ctx);
			syslog(LOG_INFO, "main bid_time_clear");
			set_timer_day(g_logid_local);
			syslog(LOG_INFO, "main set timer after");
		}
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
	bool parsesuccess = false;
	string logdir;
	bool is_textdata = false;
	bool is_print_time = false;
	char *ipbpath = NULL;
	vector<uint32_t> v_cat;
	map< uint32_t, vector<int> >::iterator ad_out;         // out
	map<int, vector<uint32_t> >::iterator ad_in;           // in
	map<int, vector<uint32_t> >::iterator app;
	map<string, uint16_t>::iterator it_make;
	map<int, vector<uint32_t> >::iterator it;
	map<uint8_t, uint8_t>::iterator attr_in;
	map<uint8_t, vector<uint8_t> >::iterator attr_out;
	map<int, int>::iterator geoipb_it;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();
	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();
	vector<pthread_t> thread_id;

	FCGX_Init();

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count < 1)
	{
		FAIL_SHOW;
		err = E_INVALID_CPU_COUNT;
		va_cout("cpu_count error!");
		run_flag = false;
		goto release;
	}

	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if (ipbpath == NULL)
	{
		FAIL_SHOW;
		err = E_INVALID_PROFILE_IPB;
		run_flag = false;
		goto release;
	}

	id = new pthread_t[cpu_count];
	if (id == NULL)
	{
		va_cout("allocate pthread_t error");
		va_cout("cpu_count=%d", cpu_count);
		FAIL_SHOW;
		err = E_MALLOC;
		run_flag = false;
		goto release;
	}
	va_cout("cpu_count=%d", cpu_count);

	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		FAIL_SHOW;
		err = E_FILE_IPB;
		run_flag = false;
		goto release;
	}

	logdir = "locallog";
	is_textdata = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "is_textdata");
	va_cout("loglocal is_textdata  %d", is_textdata);
	is_print_time = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	g_logid_local = openLog(private_conf, (char *)(logdir.c_str()), is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_local == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID_LOC;
		va_cout("open local log error");
		run_flag = false;
		goto release;
	}
	fullreqrecord = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "fullreqrecord");
	ferr_level = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "ferr_level");
	va_cout("locallog ferr_level: %d", ferr_level);

	logdir = "log";
	is_textdata = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(private_conf, (char *)(logdir.c_str()), is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID;
		va_cout("open log error");
		run_flag = false;
		goto release;
	}

	logdir = "logresponse";
	is_textdata = GetPrivateProfileInt(private_conf, (char *)(logdir.c_str()), "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid_response = openLog(private_conf, (char *)(logdir.c_str()), is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_response == 0)
	{
		FAIL_SHOW;
		err = E_INVALID_PARAM_LOGID_RESP;
		va_cout("open log response error");
		run_flag = false;
		goto release;
	}

	numcount = new int[cpu_count];
	if (numcount == NULL)
	{
		va_cout("allocate numcount error");
		FAIL_SHOW;
		err = E_MALLOC;
		run_flag = false;
		goto release;
	}
	//app
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), 
		callback_insert_pair_int, (void *)&app_cat_table, false))
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		err = E_PROFILE_INIT_APPCAT;
		run_flag = false;
		goto release;
	}
#ifdef DEBUG
	for (it = app_cat_table.begin(); it != app_cat_table.end(); ++it)
	{
		string val;
		cout << it->first << " : ";
		for (int i = 0; i < it->second.size(); i++)
		{
			cout << it->second[i] << ",";
		}

		cout << endl;
	}

	v_cat = app_cat_table[60016];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] << endl;
#endif
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), 
		callback_insert_pair_int, (void *)&adv_cat_table_adxi, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_ADVCAT;
		cflog(g_logid_local, LOGERROR, "init adxcat table failed!");
		run_flag = false;
		goto release;
	}

#ifdef DEBUG
	for (it = adv_cat_table_adxi.begin(); it != adv_cat_table_adxi.end(); ++it)
	{
		string val;
		cout << it->first << " = ";
		for (int i = 0; i < it->second.size(); i++)
		{
			cout << it->second[i] << ",";
		}

		cout << endl;
	}
	v_cat = adv_cat_table_adxi[17];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] << endl;
#endif

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), 
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_MAKE;
		cflog(g_logid_local, LOGERROR, "init make table failed!");
		run_flag = false;
		goto release;
	}
#ifdef DEBUG
	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout << "dump make: " << it_make->first << " = " << it_make->second << endl;
	}
#endif
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(CREATIVEATRR)).c_str(), 
		callback_insert_pair_attr_in, (void *)&creative_attr_in, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_CRE_ATTR;
		cflog(g_logid_local, LOGERROR, "init attr table in failed!");
		run_flag = false;
		goto release;
	}
#ifdef DEBUG
	for (attr_in = creative_attr_in.begin(); attr_in != creative_attr_in.end(); ++attr_in)
	{
		printf("dump attr_in: %d = %d\n", attr_in->first, attr_in->second);
	}
#endif
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(GEOTOIPB)).c_str(), 
		callback_insert_pair_geo_ipb, (void *)&geo_ipb, true))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_GEOIPB;
		cflog(g_logid_local, LOGERROR, "init geo_ipb table in failed!");
		run_flag = false;
		goto release;
	}
#ifdef DEBUG
	printf("geo_ipb size is %d\n ", geo_ipb.size());
	for (geoipb_it = geo_ipb.begin(); geoipb_it != geo_ipb.end(); ++geoipb_it)
	{
		printf("dump geo_ipb: %d = %d\n", geoipb_it->first, geoipb_it->second);
	}
#endif
	cout << "out attr" << endl;
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(CREATIVEATRR)).c_str(), 
		callback_insert_pair_attr_out, (void *)&creative_attr_out, true))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_CRE_ATTR;
		cflog(g_logid_local, LOGERROR, "init attr table out failed!");
		run_flag = false;
		goto release;
	}

#ifdef DEBUG
	for (attr_out = creative_attr_out.begin(); attr_out != creative_attr_out.end(); ++attr_out)
	{
		printf("dump attr_out: %d = ", attr_out->first);
		for (int i = 0; i < attr_out->second.size(); i++)
		{
			printf("%d, ", attr_out->second[i]);
		}
		cout << endl;
	}
#endif

	MD5_Init(&hash_ctx);

	err = bid_filter_initialize(g_logid_local, ADX_GOOGLE, cpu_count, &run_flag, private_conf, thread_id, &ctx);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		va_cout("bid filter initialize error!!");
		run_flag = false;
		goto release;
	}
	cflog(g_logid_local, LOGINFO, "bid_filter_initialize success! ctx:0x%x", ctx);

	err = g_ser_log.init(1, ADX_GOOGLE, g_logid_local, private_conf, &run_flag);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		run_flag = false;
		goto release;
	}

	for (uint8_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}

release:
	g_ser_log.uninit();
	if (g_logid != 0)
		closeLog(g_logid);
	if (g_logid_local != 0)
		closeLog(g_logid_local);
	if (g_logid_response != 0)
		closeLog(g_logid_response);
	if (geodb != 0)
	{
		closeIPB(geodb);
		geodb = 0;
	}

	if (ctx != 0)
	{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;
	}
	if (numcount != NULL)
	{
		delete[]numcount;
		numcount = NULL;
	}

	google::protobuf::ShutdownProtobufLibrary();

	if (err == E_SUCCESS)
	{
		for (uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}

	if (id != NULL)
	{
		delete[]id;
		id = NULL;
	}
	cout << "main exit, errcode:0x" << hex << err << endl;
	return err;
}
