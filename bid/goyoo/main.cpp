#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "goyoo_response.h"
#include "../../common/bid_filter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"
#include "../../common/errorcode.h"

#define PRIVATE_CONF  "dsp_goyoo.conf"
#define APPCATTABLE   "appcat_goyoo.txt"
#define ADVCATTABLE	  "advcat_goyoo.txt"
#define DEVMAKETABLE  "make_goyoo.txt"

using namespace std;
uint64_t ctx = 0;
uint64_t geodb = 0;
int *numcount = NULL;
pthread_t *tid = NULL;
uint64_t g_logid, g_logid_local, g_logid_response;
map<int, vector<uint32_t> > app_cat_table;
map<string, uint16_t> dev_make_table;


int cpu_count;
bool fullreqrecord = false;
uint8_t ferr_level = FERR_LEVEL_CREATIVE;
bool is_print_time = false;
MD5_CTX hash_ctx;

bool run_flag = true;

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	string senddata = "";
	int errorcode = 0;
	int rc = 0;
	FLOW_BID_INFO flow_bid(ADX_GOYOO);

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

		flow_bid.reset();
		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		va_cout("remoteaddr: %s", remoteaddr);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			uint32_t contentlength;
			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: application/x-protobuf");

			//OpenRTB: "application/x-protobuf"
			if (strcasecmp(contenttype, "application/x-protobuf"))
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
			
			if (errorcode == E_REDIS_CONNECT_INVALID)
			{
				syslog(LOG_INFO, "redis connect invalid");
				kill(getpid(), SIGTERM);
			}
			else if (errorcode != E_BAD_REQUEST)
			{
				//writeLog(g_logid_local, LOGINFO, "wait for counts_mutex");
				pthread_mutex_lock(&counts_mutex);
				//writeLog(g_logid_local, LOGINFO, "wait for counts_mutex ok");
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				//writeLog(g_logid_local, LOGINFO, "release counts_mutex");
				pthread_mutex_unlock(&counts_mutex);
				//writeLog(g_logid_local, LOGINFO, "release counts_mutex ok");
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
		g_ser_log.send_detail_log(index, flow_bid.to_string());
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
	return NULL;
}

void sigroutine(int dunno)
{ /* 信号处理例程，其中dunno将会得到信号的值 */

	switch (dunno) {

		// 	case 1:
		// 
		// 		cout<<"Get a signal -- SIGHUP "<<endl;
		// 		if (ctx != 0)
		// 		{
		// 			int errcode = bid_filter_uninitialize(ctx);
		// 			cout<<"bid_filter_uninitialize(ctx:"<<ctx<<", errcode:"<<errcode<<endl;
		// 		}
		// 		break;

	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		va_cout("Get a signal -- %d, ctx:%d, cpu_count:%d", dunno, ctx, cpu_count);
		run_flag = false;

		break;

		// 	case 3:
		// 
		// 		cout<<"Get a signal -- SIGQUIT "<<endl;
		// 		if (ctx != 0)
		// 		{
		// 			int errcode = bid_filter_uninitialize(ctx);
		// 			cout<<"bid_filter_uninitialize(ctx:"<<ctx<<", errcode:"<<errcode<<endl;
		// 		}
		// 		break;
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
	bool is_textdata = false;
	vector<uint32_t> v_cat;
	map<int, vector<uint32_t> >::iterator it;
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
	//	signal(SIGALRM, sigroutine);

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

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), 
		callback_insert_pair_string, (void *)&app_cat_table, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_APPCAT;
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		run_flag = false;
		goto exit;
	}

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

	v_cat = app_cat_table[816];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] << endl;

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), 
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		FAIL_SHOW;
		err = E_PROFILE_INIT_MAKE;
		cflog(g_logid_local, LOGERROR, "init make table failed!");
		run_flag = false;
		goto exit;
	}

	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout << "dump make: " << it_make->first << " -> " << it_make->second << endl;
	}

	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");

	/*err = set_timer_day(g_logid_local);
	if (err != 0)
	{
	goto exit;
	}*/

	MD5_Init(&hash_ctx);

	err = bid_filter_initialize(g_logid_local, ADX_GOYOO, cpu_count, &run_flag, private_conf, thread_id, &ctx);
	if (err != E_SUCCESS)
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "bid_filter_initialize failed! errcode:0x%x", err);
		run_flag = false;
		goto exit;
	}

	err = g_ser_log.init(cpu_count, ADX_GOYOO, g_logid_local, private_conf, &run_flag);
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