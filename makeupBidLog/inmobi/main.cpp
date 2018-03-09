#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <syslog.h>
#include "../../bid/inmobi/inmobi_response.h"
#include "../../common/errorcode.h"
#include "../../common/bid_filter.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF	"dsp_inmobi.conf"
#define APPCATTABLE		"appcat_inmobi.txt"
#define ADVCATTABLE		"advcat_inmobi.txt"
#define DEVMAKETABLE  "make_inmobi.txt"

char *logserver_ip = NULL;
uint16_t logserver_port = 0;

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

int main(int argc,char **argv)
{
	if(argc < 4)
	{
		cout<<"input log server IP, port and file path:"<<endl;
		return 0;
	}

	vector<uint32_t> v_cat;
	int count = 0;

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), callback_insert_pair_string_hex, (void *)&app_cat_table_adx2com, false))
	{
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		return 1;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), callback_insert_pair_string_hex, (void *)&adv_cat_table_adx2com, false))
	{
		cflog(g_logid_local, LOGERROR, "init adv_cat_table_adx2com table failed!");
		return 1;
	}

  if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		va_cout("init make table failed!!");
		return 1;
	}

     char *ipbpath = NULL;
	 ipbpath = GetPrivateProfileString((char *)(string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE)).c_str(), (char *)"default", (char *)"ipb");
	if(ipbpath == NULL)
	{
		va_cout("ipb error!!");
		return 1;
	}
	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		va_cout("open ipb error!!");
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
		while(file_in.getline(szline, 8192))
		{
			MESSAGEREQUEST mrequest;//adx msg request
			COM_REQUEST crequest;//common msg request
			strcpy(writedata, szline + 26);
			init_message_request(mrequest);
			int imptype =0;
			cout <<writedata <<endl;

			if (!parse_inmobi_request(writedata, mrequest,imptype))
			{
				cout<< "parse_inmobi_request failed! Mark errcode: E_BAD_REQUEST" <<endl;
				continue;
			}
			
			init_com_message_request(&crequest);
			transform_request(mrequest, imptype,crequest);
			if (crequest.id == "")
			{
				continue;
			}
			if ((crequest.imp.size() == 0) || (crequest.device.dids.size() == 0 && crequest.device.dpids.size() == 0))
			{
				continue;
			}
			sendLogdirect(datatransfer, crequest, ADX_INMOBI, err);
			count++;
		}
		file_in.close();
	}
	else
		cout <<"open file :" << argv[3] <<" failed !" <<endl;
	delete datatransfer;
release:
	if(g_logid != 0)
		closeLog(g_logid);
	if(g_logid_local != 0)
		closeLog(g_logid_local);
	if (g_ctx != 0)
	{
		int errcode = bid_filter_uninitialize(g_ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", g_ctx, errcode);
		g_ctx = 0;
	}
	cout <<"total send endl :" << count <<endl;
	return 0;
}
