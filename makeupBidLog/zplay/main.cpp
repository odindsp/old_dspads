#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <string>
#include <signal.h>
#include "../../bid/zplay/zplay_response.h"
using namespace std;

#define ZPLAY_CONF		"dsp_zplay.conf"
#define APPCATTABLE		"appcat_zplay.txt"

uint64_t g_logid_local, g_logid, g_logid_response;

string logserver_ip = "";
uint64_t logserver_port = 0;
uint64_t cpu_count = 0;
pthread_t *id = NULL;
uint64_t g_ctx = 0;
MD5_CTX hash_ctx;
uint64_t geodb = 0;
map<string, vector<uint32_t> > zplay_app_cat_table;
bool is_print_time = false;

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		cout<<"input the log server IP, PORT and filename:"<< endl;
		return 0;
	}

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


	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(), callback_insert_pair_string_hex, (void *)&zplay_app_cat_table, false))
	{
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


			com::google::openrtb::BidRequest bidrequest;
			if(bidrequest.ParseFromArray((char *)buf,len))
			{
				int err = 0;
				COM_REQUEST commrequest;
				com::google::openrtb::BidResponse bidresponse;
				uint8_t viewtype = 0;
				TranferToCommonRequest(bidrequest,commrequest,bidresponse);
				sendLogdirect(datatransfer,commrequest, ADX_ZPLAY, err);
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
	if (g_ctx != 0)
	{
		int errcode = bid_filter_uninitialize(g_ctx);
		va_cout("bid_filter_uninitialize(ctx:0x%x), errcode:0x%x", g_ctx, errcode);
		g_ctx = 0;
	}

	cout<<"main end"<<endl;
	return 0;
}