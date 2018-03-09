#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "../../bid/amax/amax_response.h"
#include "../../common/bid_filter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"
#include "../../common/errorcode.h"

#define PRIVATE_CONF  "dsp_amax.conf"
#define APPCATTABLE   "appcat_amax.txt"
#define ADVCATTABLE	  "advcat_amax.txt"
#define DEVMAKETABLE  "make_amax.txt"

using namespace std;
uint64_t ctx = 0;
int *numcount = NULL;
pthread_t *tid = NULL;
uint64_t geodb = 0;
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

int main(int argc,char **argv)
{
	if(argc < 4)
	{
		cout<<"input log server IP, port and file path:"<<endl;
		return 0;
	}

	vector<uint32_t> v_cat;
	int count = 0;
	char *ipbpath = NULL;
	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	map<string, vector<uint32_t> >::iterator it;
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), callback_insert_pair_string, (void *)&app_cat_table, false))
	{
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		return 1;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), callback_insert_pair_string, (void *)&adv_cat_table_adxi, false))
	{
		cflog(g_logid_local, LOGERROR, "init adxcat table failed!");
		return 1;
	}

	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if(ipbpath == NULL)
	{
		return 1;
	}

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		return 1;
	}

	cout <<"argc :" <<argc <<endl;
	char writedata[8192];
	char szline[8192];

	DATATRANSFER *datatransfer = new DATATRANSFER;
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

	fstream file_in(argv[3], ios_base::in);

	if(file_in.is_open())
	{
		int err = 0;
		int ret = 0;
		while(file_in.getline(szline, 8192))
		{
			MESSAGEREQUEST mrequest;//adx msg request
			COM_REQUEST crequest;//common msg request
			strcpy(writedata, szline + 26);
			init_message_request(mrequest);
			init_com_message_request(&crequest);
			cout <<writedata <<endl;
			ret = parseAmaxRequest(writedata, mrequest, crequest);
			printf("ret : 0x:%0x\n", ret);
			if(ret == E_INVALID_PARAM)
				err = 1;
			if (ret != E_BAD_REQUEST)//竞价打印本地日志
			{
				++count;
				sendLogdirect(datatransfer, crequest, ADX_AMAX, err);
			}
			else
				cout << "data :" << writedata <<endl;
		}
		file_in.close();
	}
	else
		cout <<"open file :" << argv[3] <<" failed !" <<endl;
release:
	if(g_logid != 0)
		closeLog(g_logid);
	if(g_logid_local != 0)
		closeLog(g_logid_local);
	if(g_server_ip != NULL)
		free(g_server_ip);
	if (ctx != 0)
	{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;
	}
	cout <<"total send endl :" << count <<endl;
	if (geodb != 0)
	{
		closeIPB(geodb);
	}
	return 0;
}
