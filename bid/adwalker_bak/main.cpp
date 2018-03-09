#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "adwalker_response.h"
#include "../../common/bid_filter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"
#include "../../common/errorcode.h"

#define PRIVATE_CONF  "dsp_adwalker.conf"
#define APPCATTABLE   "appcat_adwalker.txt"
#define ADVCATTABLE	  "advcat_adwalker.txt"
#define DEVMAKETABLE  "make_adwalker.txt"

using namespace std;
uint64_t ctx = 0;
int *numcount = NULL;
pthread_t *tid = NULL;
uint64_t g_logid, g_logid_local, g_logid_response;
map<string, vector<uint32_t> > app_cat_table;
map<string, vector<uint32_t> > adv_cat_table_adxi;
map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
map<string, uint16_t> dev_make_table;

char *g_server_ip;
uint32_t g_server_port;
int cpu_count;
bool fullreqrecord = false;
bool is_print_time = false;
MD5_CTX hash_ctx;

static void *doit(void *arg)
{
	pthread_detach(pthread_self());
	//uint64_t ctx = (uint64_t)arg;
	uint8_t index = (uint64_t)arg;
	DATATRANSFER *datatransfer = new DATATRANSFER;
	//pthread_t threadlog;
	FCGX_Request request;
	string senddata = "";
	int errorcode = 0;
	int rc = 0;

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

	datatransfer->server = g_server_ip;
	datatransfer->port = g_server_port;
	datatransfer->logid = g_logid_local;

connect_log_server:
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
	if (datatransfer->fd == -1)
	{
		va_cout("connect failed");
		writeLog(g_logid_local, LOGINFO, "Connect log server " + datatransfer->server + " failed!");
		usleep(100000);
		//goto connect_log_server;
		goto exit;
	}
	
	pthread_mutex_init(&datatransfer->mutex_data, 0);
	pthread_create(&datatransfer->threadlog, NULL, threadSendLog, datatransfer);
	
	while (1)
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

		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		va_cout("remoteaddr: %s", remoteaddr);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			uint32_t contentlength;
			/*char *version = FCGX_GetParam("HTTP_X_AMAXRTB_VERSION", request.envp);
			if (version)
			{
				va_cout("find! x-amaxrtb-version: %s", version);
			}
			else
			{
				va_cout("not find! x-amaxrtb-version! ");
				writeLog(g_logid_local, LOGINFO, "not find! x-amaxrtb-version! ");
				goto nextLoop;
			}*/

			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: %s", contenttype);

			//OpenRTB: "application/json"
			if (strcasecmp(contenttype, "application/json") &&
				strcasecmp(contenttype, "application/json; charset=utf-8"))//adwalker has "charset=utf-8", but firefox has "UTF-8"
			{
				va_cout("Wrong contenttype: %s", contenttype);
				writeLog(g_logid_local, LOGINFO, "Content-Type error.");
				goto nextLoop;
			}

			contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			va_cout("contentlength: %d", contentlength);
			if (contentlength == 0)
			{
				cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
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
					goto exit;
				}
			}

			recvdata->length = contentlength;

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				writeLog(g_logid_local, LOGINFO, "fread fail");
				goto nextLoop;
			}
			recvdata->data[recvdata->length] = 0;
			if(fullreqrecord)
				writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);

			errorcode = getBidResponse(ctx, index, datatransfer, recvdata, senddata);
			if(errorcode ==  E_SUCCESS)
			{
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

		}
		else
		{
			va_cout("Not POST.");
			writeLog(g_logid_local, LOGINFO, "Not POST.");
		}
nextLoop:
		FCGX_Finish_r(&request);
	}
exit:

	if (recvdata != NULL)
	{
		if (recvdata->data != NULL)
			free(recvdata->data);
		free(recvdata);
	}

	if (datatransfer->fd != -1)
	{
		disconnectServer(datatransfer->fd);
		pthread_mutex_destroy(&datatransfer->mutex_data);
	}	
	if(datatransfer != NULL)
		delete datatransfer;

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
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
		if (ctx != 0)
		{
			syslog(LOG_INFO, "Get a signal -- %d", dunno);
			va_cout("Get a signal -- %d, ctx:%d, cpu_count:%d", dunno, ctx, cpu_count);
			int errcode = bid_filter_uninitialize(ctx);
			va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
			ctx = 0;

			for (int i = 0; i < cpu_count; i++)
			{
				va_cout("pthread_kill[%d]:%d...", i, tid[i]);
				pthread_kill(tid[i], SIGKILL);
				va_cout("pthread_kill[%d]:%d over", i, tid[i]);
			}		
		}	
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
		if (ctx != 0)
		{
			bid_time_clear(ctx);
			set_timer_day(g_logid_local);
		}
		break;
	default:
		cout<<"Get a unknown -- "<< dunno << endl;
		break;

	}

	return;

}

int main(int argc, char *argv[])
{
	int err = 0;
	bool is_textdata = false;
	vector<uint32_t> v_cat;
	map<string, vector<uint32_t> >::iterator it;
	//map<uint32_t, vector<uint32_t> >::iterator it_i;
	map<string, uint16_t>::iterator it_make;

	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if(cpu_count == 0)
		return 1;

	tid = new pthread_t[cpu_count];
	numcount = (int *)calloc(cpu_count, sizeof(int));
	
	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);
	signal(SIGALRM, sigroutine);

	FCGX_Init();

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time  %d", is_print_time);

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);

	g_logid_local = openLog(private_conf, "locallog", is_textdata, is_print_time);
	if(g_logid_local == 0)
		goto exit;

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(private_conf, "log", is_textdata, is_print_time);
	if(g_logid == 0)
		goto exit;

	is_textdata = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");
	va_cout("logresponse is_textdata  %d", is_textdata);
	g_logid_response = openLog(private_conf, "logresponse", is_textdata, is_print_time);
	if(g_logid_response == 0)
		goto exit;

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), callback_insert_pair_string, (void *)&app_cat_table, false))
	{
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		goto exit;
	}

	for (it = app_cat_table.begin(); it != app_cat_table.end(); ++it)
	{
		string val;
		cout << it->first <<" : " ;
		for (int i = 0; i < it->second.size(); i++)
		{
			cout << it->second[i] << ",";
		}

		cout << endl;
	}

	v_cat = app_cat_table["60016"];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] <<endl;

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), callback_insert_pair_string, (void *)&adv_cat_table_adxi, false))
	{
		cflog(g_logid_local, LOGERROR, "init adxcat table failed!");
		goto exit;
	}

	for (it = adv_cat_table_adxi.begin(); it != adv_cat_table_adxi.end(); ++it)
	{
		string val;
		cout << it->first <<" : " ;
		for (int i = 0; i < it->second.size(); i++)
		{
			cout << it->second[i] << ",";
		}

		cout << endl;
	}

	v_cat = adv_cat_table_adxi["17"];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] <<endl;

	/*if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), callback_insert_pair_string_r, (void *)&adv_cat_table_adxo, true))
	{
		cflog(g_logid_local, LOGERROR, "init adxcat table failed!");
		goto exit;
	}

	for (it_i = adv_cat_table_adxo.begin(); it_i != adv_cat_table_adxo.end(); ++it_i)
	{
		string val;
		cout << it_i->first <<" : " ;
		for (int i = 0; i < it_i->second.size(); i++)
		{
			cout << it_i->second[i] << ",";
		}

		cout << endl;
	}

	v_cat = adv_cat_table_adxo[786688];
	for (uint32_t j = 0; j < v_cat.size(); j++)
		cout << "after mapping is " << v_cat[j] <<endl;
	*/

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		cflog(g_logid_local, LOGERROR, "init make table failed!");
		goto exit;
	}

	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout<< "dump make: " << it_make->first << " -> " << it_make->second << endl;
	}


	g_server_ip = GetPrivateProfileString(private_conf, "logserver", "bid_ip");
	if(g_server_ip == NULL)
		goto exit;

	g_server_port = GetPrivateProfileInt(private_conf, "logserver", "bid_port");
	if(g_server_port == 0)
		goto exit;

	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");

	err = set_timer_day(g_logid_local);
	if (err != 0)
	{
		goto exit;
	}

	MD5_Init(&hash_ctx);  

	err = bid_filter_initialize(g_logid_local, ADX_ADWALKER, cpu_count, private_conf, &ctx);
	if (err != 0)
	{
		cflog(g_logid_local, LOGERROR, "bid_filter_initialize failed! errcode:0x%x", err);
		goto exit;
	}

	tid[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; i++)
		pthread_create(&tid[i], NULL, doit, (void *)i);

	doit(0);
exit:

	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

	if(g_logid_response)
		closeLog(g_logid_response);

	if (g_server_ip)
		free(g_server_ip);

	if (ctx != 0)
	{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;
	}

	if (tid)
	{
		delete tid;
		tid = NULL;
	}

	return 0;
}
