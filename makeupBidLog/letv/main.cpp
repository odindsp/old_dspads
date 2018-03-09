#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include <iostream>
#include <fstream>
#include "../../bid/letv/letv_response.h"
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
	int count = 0;
	FCGX_Init();
	cout <<"argc :" <<argc <<endl;
	char writedata[8192];
	char szline[8192];
	fstream file_in;

	DATATRANSFER *datatransfer  = NULL;
	
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
	datatransfer = new DATATRANSFER;
	if (datatransfer == NULL)
	{
		return -1;
	}
	datatransfer->server = argv[1];
	datatransfer->port = atoi(argv[2]);
	datatransfer->logid = g_logid_local;
connect_log_server:
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
	if (datatransfer->fd == -1)
	{
		va_cout("connect IP %s and port %s failed", argv[1], argv[2]);
		usleep(100000);
		goto connect_log_server;
	}

	file_in.open(argv[3], ios_base::in);

	if(file_in.is_open())
	{
		int err = 0;
		int ret = 0;
		while(file_in.getline(szline, 8192))
		{
			COM_REQUEST crequest;//common msg request
			strcpy(writedata, szline + 26);
			init_com_message_request(&crequest);
			cout <<writedata <<endl;
			ret = parseLetvRequest(writedata, crequest);
			printf("ret : 0x:%0x\n", ret);
			if(ret == E_INVALID_PARAM)
				err = 1;
			if (ret != E_BAD_REQUEST)//竞价打印本地日志
			{
				++count;
				sendLogdirect(datatransfer, crequest, ADX_LETV, err);
			}
			else
				cout << "data :" << writedata <<endl;
		}
		file_in.close();
	}
	else
		cout <<"open file :" << argv[3] <<" failed !" <<endl;

	cout <<"total send endl :" << count <<endl;
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