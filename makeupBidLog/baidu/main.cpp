#include <sys/stat.h>
#include <iostream>
#include "../../bid/baidu/baidu_adapter.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include <stdlib.h>
#include "../../common/setlog.h"
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <map>
#include <vector>
#include "../../common/confoperation.h"
#include "../../bid/baidu/baidu_realtime_bidding.pb.h"
#include <signal.h>
#include <syslog.h>
using namespace std;

#define PRIVATE_CONF          "dsp_baidu.conf"
#define AD_CATEGORY_FILE     "advcat_baidu.txt"
#define AD_APP_FILE          "appcat_baidu.txt"
#define DEVMAKETABLE         "make_baidu.txt"

uint64_t g_logid, g_logid_local, g_logid_response;
char *g_server_ip = NULL;
uint32_t g_server_port;
uint64_t ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;
map< uint32_t,vector<int> > outputadcat;         // out
map<int,vector<uint32_t> > inputadcat;           // in
map<int,vector<uint32_t> > appcattable;
map<string, uint16_t> dev_make_table;
uint64_t geodb = 0;

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

	DATATRANSFER *datatransfer = new DATATRANSFER;

	if (datatransfer == NULL)
	{
		return -1;
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


	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_APP_FILE)).c_str(), transfer_adv_int, (void *)&appcattable,false))
	{
		va_cout("init app cat error!!");
		goto release;
	}


	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(), transfer_adv_int, (void *)&inputadcat, false))
	{
		va_cout("init ad in cat error!!");
		goto release;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		va_cout("init make table failed!!");
		goto release;
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
				BidResponse bidresponse;
				parse_common_request(bidrequest,commrequest,bidresponse);
				sendLogdirect(datatransfer,commrequest,ADX_BAIDU,err);
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

