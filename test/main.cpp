#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <queue>
#include "impclk.h"
using namespace std;
extern "C"
{
#include <dlfcn.h>
	//  #include <ccl/ccl.h>
}

#define 	SLOGCONF			"dsp_settle.conf"
#define 	LOGCONF				"dsp_impclk.conf"
#define		PRICECONF			"dsp_price.conf"

char success_msg[4096] = "Status: 200 OK\r\nContent-Type: image/gif; charset=utf-8\r\nConnection: close\r\n\r\n";//will be update.
int success_msg_len = 0;//will be update.

uint64_t g_logid_local = 0;
pthread_t *tid = NULL;
uint8_t cpu_count = 0;

char *impclk_ip;
int impclk_port;

void *threadSendLog_s(void *arg)
{
	int errorcode = 0;
	DATATRANSFER *datatransfer = (DATATRANSFER *)arg;
	long lasttime = 0;
	int index = 0, size = 0;

	pthread_detach(pthread_self());

	while (1)
	{
		pthread_mutex_lock(&datatransfer->mutex_data);
		if (!datatransfer->data.empty())
		{
			string logdata;

			index++;
			size = datatransfer->data.size();
			logdata = datatransfer->data.front();
			datatransfer->data.pop();
			pthread_mutex_unlock(&datatransfer->mutex_data);

			errorcode = sendMessage(datatransfer->fd, logdata.data(), logdata.size());
			if (errorcode <= 0)
			{
				datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
				writeLog(datatransfer->logid, LOGDEBUG, "reconnect");
			}
		}
		else
		{
			pthread_mutex_unlock(&datatransfer->mutex_data);
			usleep(100000);
		}
	}

	return NULL;
}

static void *doit(void *arg)
{
	FCGX_Request request;
	DATATRANSFER *datatransfer = new DATATRANSFER;
	//pthread_t threadlog;
	int rc = 0;

	char *remoteaddr = NULL, *requesturi = NULL, *requestparam = NULL;
	string senddata = string(success_msg, success_msg_len);

	pthread_detach(pthread_self());

	FCGX_InitRequest(&request, 0, 0);

	datatransfer->logid = g_logid_local;
	datatransfer->server = string(impclk_ip);
	datatransfer->port = impclk_port;

connect_log_server:
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
	if (datatransfer->fd == -1)
	{
		writeLog(g_logid_local, LOGERROR, "Connect impclk server " + datatransfer->server + " failed!");
		//usleep(100000);
		goto connect_log_server;
	}

	pthread_mutex_init(&datatransfer->mutex_data, 0);
	pthread_create(&datatransfer->threadlog, NULL, threadSendLog, datatransfer);

	for(;;)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;		
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		int errcode = E_SUCCESS;

		/* Accept the request message */
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		writeLog(g_logid_local, LOGINFO,"rc:%d", rc);
		if(rc < 0)
		{
			writeLog(g_logid_local, LOGERROR, "FCGX_Accept_r failed,at:%s,in:%d", __FUNCTION__, __LINE__);
			goto fcgi_finish;
		}

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		requesturi = FCGX_GetParam("REQUEST_URI", request.envp);
		requestparam = FCGX_GetParam("QUERY_STRING", request.envp);

		if((remoteaddr != NULL) && (requesturi != NULL) && (requestparam != NULL))
		{
			writeLog(g_logid_local, LOGDEBUG, string(remoteaddr) + " " + string(requestparam));
			errcode = parseRequest(request, datatransfer, remoteaddr, requesturi, requestparam);
		}
		else
			writeLog(g_logid_local, LOGERROR, "remoteaddr or requesturi or requestparam is NULL!");

		if (errcode == E_SUCCESS)
		{
			pthread_mutex_lock(&context_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&context_mutex);
		}
		else
			writeLog(g_logid_local, LOGERROR, "request invalid,err:0x%x,at:%s,in:%d", E_INVALID_PARAM_REQUEST, __FUNCTION__, __LINE__);

fcgi_finish:
		FCGX_Finish_r(&request);
	}

exit:
	disconnectServer(datatransfer->fd);
	pthread_mutex_destroy(&datatransfer->mutex_data);
	delete datatransfer;
}

bool make_success_message()
{
 	char gif[64] = {
		0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0xff, 0x00, 0xff,
 		0x00, 0x00, 0x00, 0x21, 0xf9, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00,
 		0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3b, 0x00};
	
	
	success_msg_len = strlen(success_msg);

	memcpy(success_msg + success_msg_len, gif, 44);

	success_msg_len += 44;
	
	return true;
}

int main(int argc, char *argv[])
{
	pthread_t thread_id, thread_ur, thread_group, thread_group_wr, thread_price, thread_write_aprice;


	int errcode = E_SUCCESS;

	string impclk_conf = "", global_conf = "";

	global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	impclk_conf = string(GLOBAL_PATH) + string(LOGCONF);

	char *sz_global_conf = (char *)global_conf.c_str();
	char *sz_impclk_conf = (char *)impclk_conf.c_str();


	FCGX_Init();

	cpu_count = GetPrivateProfileInt(sz_global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		errcode = E_INVALID_PROFILE;
		goto exit;
	}

	tid = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (tid == NULL)
	{
		errcode = E_MALLOC;
		va_cout("id malloc failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	impclk_ip = GetPrivateProfileString(sz_global_conf, "logserver", "imp_clk_ip");
	impclk_port = GetPrivateProfileInt(sz_global_conf, "logserver", "imp_clk_port");


	if ((impclk_ip == NULL) || (impclk_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
	
	g_logid_local = openLog(sz_impclk_conf, "locallog", true);
	if((g_logid_local == 0))
	{
		errcode = E_INVALID_PARAM_LOGID;
		va_cout("openlog failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	make_success_message();

	tid[0] = pthread_self();

	for (uint8_t i = 1; i < cpu_count; i++)
		pthread_create(&tid[i], NULL, doit, (void *)i);

	doit(0);

exit:
	if(g_logid_local)
		closeLog(g_logid_local);

	cout << "exit,errcode:" << errcode << endl;

	return errcode;
}
