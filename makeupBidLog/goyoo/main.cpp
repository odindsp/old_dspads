#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "../../bid/goyoo/goyoo_response.h"
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
int *numcount = NULL;
pthread_t *tid = NULL;
uint64_t geodb = 0;
uint64_t g_logid, g_logid_local, g_logid_response;
map<int, vector<uint32_t> > app_cat_table;
map<string, uint16_t> dev_make_table;

char *g_server_ip;
uint32_t g_server_port;
int cpu_count;
bool fullreqrecord = false;
uint8_t ferr_level = FERR_LEVEL_CREATIVE;
bool is_print_time = false;
MD5_CTX hash_ctx;

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		cout<<"input the log server IP, PORT and filename:"<< endl;
		return 0;
	}

	map< uint32_t,vector<int> >::iterator ad_out;         // out
	map<int,vector<uint32_t> >::iterator ad_in;           // in
	map<int,vector<uint32_t> >::iterator app;
	char *ipbpath = NULL;
	DATATRANSFER *datatransfer = new DATATRANSFER;
	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	if (datatransfer == NULL)
	{
		return -1;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), callback_insert_pair_string, (void *)&app_cat_table, false))
	{
		cflog(g_logid_local, LOGERROR, "init appcat table failed!");
		goto release;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		cflog(g_logid_local, LOGERROR, "init make table failed!");
		goto release;
	}

	
	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if(ipbpath == NULL)
	{
		return 1;
	}

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		return -1;
	}

	datatransfer->server = argv[1];
	datatransfer->port = atoi(argv[2]);
connect_log_server:
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
	if (datatransfer->fd == -1)
	{ 
		va_cout("conn error");
		usleep(100000);
		goto connect_log_server;
	}
	else
		va_cout("success");

	if(1)
	{
		ifstream out;
		out.open(argv[3],ios_base::in|ios_base::binary);
		if(!out.is_open())
		{
			cout<<"open file error"<<endl;
			return 0;
		}
		unsigned char buf[10240];
		bzero(buf,10240);
		unsigned char slen[4];
		unsigned int len = 0 ;
		while(!out.eof())
		{
			if(!out.read((char *)slen,4))
				break;
			len = 0;
			for(int i = 0; i < 4; ++i)
			{
				len += (slen[i]<<(24 - i*8));
			}
			out.seekg(8, ios::cur);
			out.read((char *)buf, len);


			BidRequest bidrequest;
			if(bidrequest.ParseFromArray((char *)buf,len))
			{
				int err = 0;
				COM_REQUEST commrequest;
				init_com_message_request(&commrequest);
				BidResponse bidresponse;
				parseGoyooRequest(bidrequest, commrequest, bidresponse);
				sendLogdirect(datatransfer,commrequest, ADX_GOYOO, err);
			}
			else
			{
				cout<<"parse fail!"<<endl;
			}
			out.seekg(1,ios::cur);
			bzero(buf,10240);
		}
	}

	if (datatransfer != NULL)
		delete datatransfer;

release:
	if(g_logid != 0)
		closeLog(g_logid);
	if(g_logid_local != 0)
		closeLog(g_logid_local);
	if(g_server_ip != NULL)
		free(g_server_ip);

	if (geodb != 0)
	{
		closeIPB(geodb);
	}
	if (ctx != 0)
	{
		int errcode = bid_filter_uninitialize(ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", ctx, errcode);
		ctx = 0;
	}

	cout<<"main end"<<endl;
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}