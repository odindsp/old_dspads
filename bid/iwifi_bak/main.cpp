#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "iwifi_response.h"
#include "../../common/bid_filter.h"
#include "../../common/setlog.h"
#include "../../common/confoperation.h"
#include "../../common/type.h"

#define PRIVATE_CONF "/etc/dspads/dsp_iwifi.conf"

using namespace std;
uint64_t ctx = 0;
int *numcount = NULL;
pthread_t *tid = NULL;
uint64_t g_logid, g_logid_local;
map< int, vector<int> > amaxadcategory;
char *g_server_ip;
uint32_t g_server_port;
int cpu_count;
bool fullreqrecord = false;

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
//	char *remoteaddr, *contenttype, *version;
//	uint32_t contentlength = 0;

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
		goto connect_log_server;
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
			char *version = FCGX_GetParam("HTTP_X_AMAXRTB_VERSION", request.envp);
			if (version)
			{
				va_cout("find! x-amaxrtb-version: %s", version);
			}
			else
			{
				va_cout("not find! x-openrtb-version! ");
				writeLog(g_logid_local, LOGINFO, "not find! x-openrtb-version! ");
				continue;
			}

			char *contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
			va_cout("request contenttype: %s", contenttype);

			//OpenRTB: "application/json"
			if (strcasecmp(contenttype, "application/json") &&
				strcasecmp(contenttype, "application/json; charset=utf-8"))//amax has "charset=utf-8", but firefox has "UTF-8"
			{
				va_cout("Wrong contenttype: %s", contenttype);
				writeLog(g_logid_local, LOGINFO, "Content-Type error.");
				continue;
			}

			uint32_t contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
			va_cout("contentlength: %d", contentlength);
			if (contentlength == 0)
			{
				cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
				continue;
			}
			//		cflog(g_logid_local, LOGINFO, "CONTENT_LENGTH: %d", contentlength);
			if (contentlength >= recvdata->buffer_length)
			{
				cflog(g_logid_local, LOGERROR, "CONTENT_LENGTH is too big!!!!!!! contentlength:%d! realloc recvdata.", contentlength);

				recvdata->buffer_length = contentlength;
				recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length + 1);
				if (recvdata->data == NULL)
				{
					cflog(g_logid_local, LOGERROR, "recvdata->data: realloc memory failed!");
					goto exit;
				}
			}

			recvdata->length = contentlength;
			/*RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
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
			}*/

			if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
			{
				free(recvdata->data);
				free(recvdata);
				writeLog(g_logid_local, LOGINFO, "fread fail");
				continue;
			}
			recvdata->data[recvdata->length] = 0;
			//writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);

			errorcode = getBidResponse(ctx, index, datatransfer, recvdata, senddata);
			if(errorcode ==  0)
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
			va_cout("Not POST.");
			writeLog(g_logid_local, LOGINFO, "Not POST.");
		}

		FCGX_Finish_r(&request);
	}
exit:

	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);

	if (datatransfer->fd != -1)
	{
		disconnectServer(datatransfer->fd);
		pthread_mutex_destroy(&datatransfer->mutex_data);
		delete datatransfer;
	}	

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
			syslog(LOG_INFO, "Get a signal -- %d", dunno);
		va_cout("Get a signal -- %d, ctx:%d, cpu_count:%d", dunno, ctx, cpu_count);
		if (ctx != 0)
		{
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
	default:
		cout<<"Get a unknown -- "<< dunno << endl;
		break;

	}

	return;

}

int main(int argc, char *argv[])
{
	cpu_count = GetPrivateProfileInt(GLOBAL_CONF, "default", "cpu_count");
	if(cpu_count == 0)
		return 1;

	tid = new pthread_t[cpu_count];
	numcount = (int *)calloc(cpu_count, sizeof(int));
	int err = 0;
	bool is_textdata = false;
	
	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	FCGX_Init();
	is_textdata = GetPrivateProfileInt(PRIVATE_CONF, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);
	g_logid_local = openLog(PRIVATE_CONF, "locallog", is_textdata);
	if(g_logid_local == 0)
		goto exit;

	is_textdata = GetPrivateProfileInt(PRIVATE_CONF, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);
	g_logid = openLog(PRIVATE_CONF, "log", is_textdata);
	if(g_logid == 0)
		goto exit;

	g_server_ip = GetPrivateProfileString(PRIVATE_CONF, "logserver", "bid_ip");
	if(g_server_ip == NULL)
		goto exit;

	g_server_port = GetPrivateProfileInt(PRIVATE_CONF, "logserver", "bid_port");
	if(g_server_port == 0)
		goto exit;


	err = bid_filter_initialize(g_logid_local, ADX_IWIFI, cpu_count, &ctx);
	if (err != 0)
	{
		cflog(g_logid_local, LOGERROR, "bid_filter_initialize failed! errcode:0x%x", err);
		goto exit;
	}

	fullreqrecord = GetPrivateProfileInt(PRIVATE_CONF, "log", "fullreqrecord");

	tid[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; i++)
		pthread_create(&tid[i], NULL, doit, (void *)i);

	doit(0);
exit:

	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

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
