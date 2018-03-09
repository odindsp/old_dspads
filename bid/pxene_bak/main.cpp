#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "pxene_response.h"
#include "../../common/bid_filter.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF	"bid.conf"

char *logserver_ip = NULL;
uint16_t logserver_port = 0;

uint64_t g_logid, g_logid_local;

static void *doit(void *arg)
{
	uint64_t ctx = (uint64_t)arg;
	FCGX_Request request;
	DATATRANSFER *datatransfer = new DATATRANSFER;
	pthread_t threadlog = 0;
	string senddata = "";
	int rc = 0;

	pthread_detach(pthread_self());

	FCGX_InitRequest(&request, 0, 0);

	datatransfer->logid = g_logid_local;
	datatransfer->server = logserver_ip;
	datatransfer->port = logserver_port;
/*
connect_log_server:
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
	if (datatransfer->fd == -1)
	{
		writeLog(g_logid_local, LOGINFO, "Connect log server " + datatransfer->server + " failed!");
		usleep(100000);
		goto connect_log_server;
	}
	
	pthread_mutex_init(&datatransfer->mutex_data, 0);
	pthread_create(&threadlog, NULL, threadSendLog, datatransfer);
*/	

	cout<<"doit:: arg:" + intToString((uint64_t)arg)<<endl;

	while (1)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		/* Some platforms require accept() serialization, some don't.. */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if (rc < 0)
			break;

		char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		cout<<"remoteaddr: "<<remoteaddr<<endl;

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			char *version = FCGX_GetParam("HTTP_X_OPENRTB_VERSION", request.envp);
			if (version)
			{
				cout << "find! version: " << version << endl;
			}

			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			cout << "request contenttype: " << contenttype << endl;

			//OpenRTB: "application/json"
			if (strcasecmp(contenttype, "application/json") &&
				strcasecmp(contenttype, "application/json; charset=utf-8"))//firefox has "UTF-8"
			{
				cout << "Wrong contenttype: " << contenttype << endl;
				writeLog(g_logid_local, LOGINFO, "Content-Type error.");
				continue;
			}

			uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			cout << "contentlength: " << contentlength << endl;

			RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
			if (recvdata == NULL)
			{
				writeLog(g_logid_local, LOGINFO, "recvdata:alloc memory failed!");
				continue;
			}

			recvdata->length = contentlength;
			recvdata->data = (char *)calloc(1, recvdata->length + 1);
			if (recvdata->data == NULL)
			{
				writeLog(g_logid_local, LOGINFO, "recvdata->data:alloc memory failed!");
				continue;
			}

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				writeLog(g_logid_local, LOGINFO, "fread fail");
				continue;
			}
			cout << "recvdata: " << (string)recvdata->data << endl;

			pxeneResponse(ctx, datatransfer, recvdata, senddata);
			cflog(g_logid_local, LOGINFO, "at fn:%s, ln:%d.", __FUNCTION__, __LINE__);

			pthread_mutex_lock(&counts_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&counts_mutex);
			cflog(g_logid_local, LOGINFO, "at fn:%s, ln:%d.", __FUNCTION__, __LINE__);
			cout << "senddata: " << senddata << endl;

			if (recvdata->data != NULL)
				free(recvdata->data);
			if (recvdata != NULL)
				free(recvdata);
		}
		else
		{
			cout <<"Not POST."<< endl;
			writeLog(g_logid_local, LOGINFO, "Not POST.");
		}

		FCGX_Finish_r(&request);
	}

	disconnectServer(datatransfer->fd);
	pthread_mutex_destroy(&datatransfer->mutex_data);
	free(datatransfer);
}

void sigroutine(int dunno)
{ /* 信号处理例程，其中dunno将会得到信号的值 */

	switch (dunno) {

	case 1:

		cout<<"Get a signal -- SIGHUP "<<endl;

		break;

	case 2:

		cout<<"Get a signal -- SIGINT "<<endl;
	
		break;

	case 3:

		cout<<"Get a signal -- SIGQUIT "<<endl;

		break;

	}

	return;

}

int main(int argc, char *argv[])
{
	uint64_t ctx = 0;

	const uint8_t cpu_count = GetPrivateProfileInt(GLOBAL_CONF, "default", "cpu_count");
	cout << "cpu_count:" <<cpu_count<< endl;

	pthread_t id[cpu_count];

//	signal(SIGHUP, sigroutine); //* 下面设置三个信号的处理方法

//    signal(SIGINT, sigroutine);

 //   signal(SIGQUIT, sigroutine);

	logserver_ip = GetPrivateProfileString(PRIVATE_CONF, "logserver", "bid_ip");
	if (logserver_ip != NULL)//must be free.
		cout << "logserver_ip:" << logserver_ip << endl;

	logserver_port = GetPrivateProfileInt(PRIVATE_CONF, "logserver", "bid_port");
	cout << "logserver_port:" << logserver_port << endl;

	FCGX_Init();
	cout << "FCGX_Init: completed! line:" << __LINE__ << endl;

	g_logid = openLog(PRIVATE_CONF, "log", true);
	cout << "openLog("<<PRIVATE_CONF<<":log) returned, g_logid:" << g_logid << endl;

	g_logid_local = openLog(PRIVATE_CONF, "locallog", true);
	cout << "openLog("<<PRIVATE_CONF<<":locallog) returned, g_logid:" << g_logid_local << endl;

	if (bid_filter_initialize(g_logid_local, &ctx) < 0)
		return -1;
	cout << "bid_filter_initialize success! ctx:" <<ctx<< endl;

	for (uint8_t i = 1; i < cpu_count; i++)
		pthread_create(&id[i], NULL, doit, (void *)ctx);

	doit((void *)ctx);

	bid_filter_uninitialize(ctx);

	if (logserver_ip)
		free(logserver_ip);
	
	closeLog(g_logid);
	closeLog(g_logid_local);

	return 0;
}
