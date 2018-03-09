#include <sys/stat.h>
#include <iostream>
#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <map>
#include <vector>
#include "../../common/confoperation.h"
#include "../../common/errorcode.h"
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "tanx-bidding.pb.h"
#include "tanx_adapter.h"
using namespace std;

#define PRIVATE_CONF          "dsp_tanx.conf"
#define AD_CATEGORY_FILE     "advcat_tanx.txt"
#define AD_APP_FILE          "appcat_tanx.txt"
#define DEVMAKETABLE         "make_tanx.txt"

uint64_t g_logid, g_logid_local, g_logid_response;
uint64_t ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;
map<int, vector<uint32_t> > appcattable;
map< uint32_t, vector<int> > outputadcat;         // out
map<int, vector<uint32_t> > inputadcat;           // in
map<string, uint16_t> dev_make_table;
uint8_t ferr_level = FERR_LEVEL_CREATIVE;
MD5_CTX hash_ctx;
uint64_t geodb = 0;
int *numcount = NULL;

bool run_flag = true;

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	FCGX_InitRequest(&request, 0, 0);
	int rc;
	FLOW_BID_INFO flow_bid(ADX_TANX);

	while (run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		MY_DEBUG;
		pthread_mutex_lock(&accept_mutex);
		MY_DEBUG;
		rc = FCGX_Accept_r(&request);
		MY_DEBUG;
		pthread_mutex_unlock(&accept_mutex);

		MY_DEBUG;
		if (rc < 0)
			break;

		MY_DEBUG;
		flow_bid.reset();
		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		va_cout("remoteaddr:%s", remoteaddr);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			if (strcmp(FCGX_GetParam("CONTENT_TYPE", request.envp), "application/octet-stream") != 0)
			{
				writeLog(g_logid_local, LOGERROR, "Content-Type error.");
				flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
				goto finish;
			}
			uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			if (contentlength == 0)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata length is zero!");
				flow_bid.set_err(E_REQUEST_HTTP_CONTENT_LEN);
				goto finish;
			}

			va_cout("contentlength:%d", contentlength);
			RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
			if (recvdata == NULL)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata:alloc memory failed!");
				flow_bid.set_err(E_MALLOC);
				goto finish;
			}
			MY_DEBUG;

			recvdata->length = contentlength;
			recvdata->data = (char *)calloc(1, recvdata->length + 1);
			if (recvdata->data == NULL)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata->data:alloc memory failed!");
				free(recvdata);
				recvdata = NULL;
				flow_bid.set_err(E_MALLOC);
				goto finish;
			}

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				recvdata->data = NULL;
				recvdata = NULL;
				writeLog(g_logid_local, LOGERROR, "fread fail");
				flow_bid.set_err(E_REQUEST_HTTP_BODY);
				goto finish;
			}

			//va_cout("recvlen:%d",recvdata->length);
			//va_cout("recvdata:%s",recvdata->data);
			if (fullreqrecord)
				writeLog(g_logid_local, LOGDEBUG, "recvdata= " + string(recvdata->data, recvdata->length));
			MY_DEBUG;
			string senddata;
			tanx_adapter(ctx, index, recvdata, senddata, flow_bid);

			if (senddata.length() > 0)
			{
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(senddata.data(), senddata.length(), request.out);
				pthread_mutex_unlock(&counts_mutex);
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
			writeLog(g_logid_local, LOGERROR, "Not POST.");
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
		}

	finish:
		g_ser_log.send_detail_log(index, flow_bid.to_string());
		FCGX_Finish_r(&request);
	}

exit:
	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
	cout << "doit " << (int)index << " exit" << endl;
	return NULL;
}

void sigroutine(int dunno)
{
	switch (dunno)
	{
	case SIGINT:
	case SIGTERM:
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
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
	void *status = NULL;
	bool parsesuccess = false;
	string logdir;
	bool is_textdata = false;
	bool is_print_time = false;
	map< uint32_t, vector<int> >::iterator ad_out;         // out
	map<int, vector<uint32_t> >::iterator ad_in;           // in
	map<string, uint16_t>::iterator it_make;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();
	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();
	char *ipbpath = NULL;
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

	numcount = (int *)calloc(cpu_count, sizeof(int));
	if (numcount == NULL)
	{
		FAIL_SHOW;
		err = E_MALLOC;
		run_flag = false;
		goto release;
	}

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

	MD5_Init(&hash_ctx);
	err = bid_filter_initialize(g_logid_local, ADX_TANX, cpu_count, &run_flag, private_conf, thread_id, &ctx);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		va_cout("bid filter initialize error!!");
		run_flag = false;
		goto release;
	}

	/* parsesuccess = initialize_tanx_category(AD_CATEGORY_FILE);
	 if(!parsesuccess)
	 {
	 va_cout("parse cate error!!");
	 writeLog(g_logid_local,LOGERROR,"parese cate error!");
	 goto release;
	 }
	 */
	MY_DEBUG;
	// app
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_APP_FILE)).c_str(),
		transfer_adv_int, (void *)&appcattable, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_APPCAT;
		va_cout("init app cat error!!");
		run_flag = false;
		goto release;
	}
	for (ad_in = appcattable.begin(); ad_in != appcattable.end(); ++ad_in)
	{
		cout << ad_in->first << " = ";
		vector<uint32_t> &ad = ad_in->second;
		for (int i = 0; i < ad.size(); ++i)
		{
			printf("0x%x,", ad[i]);
		}
		cout << endl;
	}

	MY_DEBUG;
	// ad
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(),
		transfer_adv_int, (void *)&inputadcat, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_ADVCAT;
		va_cout("init ad in cat error!!");
		run_flag = false;
		goto release;
	}
	MY_DEBUG;
	for (ad_in = inputadcat.begin(); ad_in != inputadcat.end(); ++ad_in)
	{
		cout << ad_in->first << " = ";
		vector<uint32_t> &ad = ad_in->second;
		for (int i = 0; i < ad.size(); ++i)
		{
			printf("0x%x,", ad[i]);
		}
		cout << endl;
	}
	MY_DEBUG;
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(),
		transfer_adv_hex, (void *)&outputadcat, true))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_ADVCAT;
		va_cout("init ad out cat error!!");
		run_flag = false;
		goto release;
	}
	MY_DEBUG;
	for (ad_out = outputadcat.begin(); ad_out != outputadcat.end(); ++ad_out)
	{
		printf("0x%x = ", ad_out->first);
		vector<int> &ad = ad_out->second;
		for (int i = 0; i < ad.size(); ++i)
		{
			printf("%d,", ad[i]);
		}
		cout << endl;
	}
	MY_DEBUG;
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(),
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_MAKE;
		va_cout("init make table failed!!");
		run_flag = false;
		goto release;
	}

	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout << "dump make: " << it_make->first << " -> " << it_make->second << endl;
	}

	err = g_ser_log.init(cpu_count, ADX_TANX, g_logid_local, private_conf, &run_flag);
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
	if (numcount)
	{
		free(numcount);
		numcount = NULL;
	}
	if (ctx != 0)
	{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;
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
