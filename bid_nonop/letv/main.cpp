#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include <iostream>
#include "letv_response.h"
using namespace std;

#define LETV_CONF 			"dsp_letv.conf"
#define LETV_MAKE_CONF		"make_letv.txt"
#define ADVCATTABLE			"advcat_letv.txt"

uint64_t g_logid_response, g_logid_local, g_logid;

string logserver_ip = "";
uint64_t logserver_port = 0;
uint64_t cpu_count = 0;
pthread_t *id = NULL;
uint64_t g_ctx = 0;
uint64_t geodb = 0;

bool fullreqrecord = false;
map<string, vector<uint32_t> > adv_cat_table_adxi;
MD5_CTX hash_ctx;

map<string, uint16_t> letv_make_table;

static void *doit(void *arg)
{
	int errcode = E_SUCCESS;
	uint8_t index = (uint64_t)arg;
	DATATRANSFER *datatransfer = new DATATRANSFER;
	pthread_t threadlog;
	FCGX_Request request;
	char *remoteaddr, *contenttype, *version;
	uint32_t contentlength = 0;
	string senddata = "";
	int rc;

	pthread_detach(pthread_self());

	FCGX_InitRequest(&request, 0, 0);

	pthread_mutex_init(&datatransfer->mutex_data, 0);
	pthread_create(&threadlog, NULL, threadSendLog, datatransfer);

	for (;;)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		RECVDATA *recvdata = NULL;

		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if (rc < 0)
			goto fcgi_finish;

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			recvdata = (RECVDATA *)malloc(sizeof(RECVDATA));
			if (recvdata == NULL)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata:alloc memory failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				goto fcgi_finish;
			}

			recvdata->length = contentlength;
			recvdata->data = (char *)calloc(1, contentlength + 1);
			if (recvdata->data == NULL)
			{
				writeLog(g_logid_local, LOGERROR, "recvdata->data:alloc memory failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				goto fcgi_finish;
			}

			if (FCGX_GetStr(recvdata->data, contentlength, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				recvdata->data = NULL;
				recvdata = NULL;
				writeLog(g_logid_local, LOGERROR, "FCGX_GetStr failed,at:%s,in:%d", __FUNCTION__, __LINE__);
				goto fcgi_finish;
			}

			errcode = letvResponse(index, g_ctx, datatransfer, recvdata, senddata);

			if (errcode != E_BAD_REQUEST)
			{
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(senddata.data(), senddata.size(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}
		}
		else
			writeLog(g_logid_local, LOGERROR, "Not POST.");

fcgi_finish:
		FCGX_Finish_r(&request);
	}

exit:

	pthread_mutex_destroy(&datatransfer->mutex_data);
	delete datatransfer;
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
	
	letv_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	global_conf = (char *)letv_global_conf.c_str();
	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		err = E_INVALID_PROFILE;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		goto exit;
	}
	
	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if(ipbpath == NULL)
	{
		err = E_INVALID_PROFILE;
		goto exit;
	}

	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		err = E_FILE_IPB;
		goto exit;
	}

	letv_private_conf = string(GLOBAL_PATH) + string(LETV_CONF);
	private_conf = (char *)letv_private_conf.c_str();
	logserver_ip = GetPrivateProfileString(private_conf, "logserver", "bid_ip");
	logserver_port = GetPrivateProfileInt(private_conf, "logserver", "bid_port");
	if ((logserver_ip == "") || (logserver_port == 0))
	{
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		err = E_INVALID_PROFILE;
		goto exit;
	}
	
	letv_make = string(GLOBAL_PATH) + string(LETV_MAKE_CONF);
	
	if (!init_category_table_t(letv_make, callback_insert_pair_make, (void *)&letv_make_table, false))
	{
		err = E_INVALID_PROFILE;
		return err;
	}
#ifdef DEBUG
	for (it = letv_make_table.begin(); it != letv_make_table.end(); ++it)
	{
		cout<< "dump make: " << it->first << " -> " << it->second << endl;
	}
#endif
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), callback_insert_pair_string, (void *)&adv_cat_table_adxi, false))
	{
		va_cout("init adxcat table failed!");
		err = E_INVALID_PROFILE;
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

	g_logid_response = openLog(private_conf, "logresponse", is_textdata, is_print_time);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, is_print_time);

	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);

	g_logid = openLog(private_conf, "log", is_textdata, is_print_time);
	if ((g_logid_response == 0) || (g_logid_local == 0) || (g_logid == 0))
	{
		err = E_INVALID_PROFILE;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", err, __FUNCTION__, __LINE__);
		goto exit;
	}

	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");

	id[0] = pthread_self();
	for (uint64_t i = 1; i < cpu_count; ++i)
		pthread_create(&id[i], NULL, doit, (void *)i);
	doit(0);
	
exit:
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

	if(ipbpath != NULL)
	{
		free(ipbpath);
	}

	va_cout("exit,errcode: 0x%x", err);
	return err;
}
