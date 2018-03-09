//date:2016-06-02
//author:wangjiao
//对URL请求分析并保存、发送本地日志(请求主要分为访问，跳出，二跳)

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <queue>
#include "pv.h"

using namespace std;

#define		MONITORCONF		"dsp_pv.conf"

char success_msg[4096] = "Status: 200 OK\r\nContent-Type: image/gif; charset=utf-8\r\nConnection: close\r\n\r\n";
int success_msg_len = 0;

uint64_t g_logid_local = 0, g_logid = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
char *pv_ip = NULL;
uint16_t pv_port = 0;
uint64_t geodb = 0;
DATATRANSFER *datatransfer = NULL; 	
bool run_flag = true;		//表示当前线程正在执行


void sigroutine(int dunno)
{
	switch (dunno)
	{
	case SIGINT:			//SIGINT
	case SIGTERM:			//SIGTERM
		run_flag = false;
		break;
	default:
		break;
	}
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

static void *doit(void *arg)
{
	FCGX_Request request;
	int rc = 0;
	char *remoteaddr = NULL, *requestparam = NULL;
#ifdef DEBUG	
	cout<<"success_msg is :"<<success_msg<<endl;
	cout<<"success_msg_len is :"<<success_msg_len<<endl;
#endif
	string senddata = string(success_msg, success_msg_len);
#ifdef DEBUG	
	cout<<"senddata is :"<<senddata<<endl;
#endif
	FCGX_InitRequest(&request, 0, 0);

	while(run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;		
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		int errcode = E_SUCCESS;

		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		writeLog(g_logid_local, LOGINFO,"rc:%d", rc);
		if(rc < 0)
		{
			writeLog(g_logid_local, LOGERROR, "FCGX_Accept_r failed");
			goto fcgi_finish;
		}
					
		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		requestparam = FCGX_GetParam("QUERY_STRING", request.envp);		
		if((remoteaddr != NULL) && (requestparam != NULL))
		{
			writeLog(g_logid_local, LOGDEBUG, string(remoteaddr) + " " + string(requestparam));
			errcode = parseRequest(request, datatransfer, remoteaddr, requestparam);
		}
		else
			writeLog(g_logid_local, LOGERROR, "remoteaddr or requestparam is NULL!");
#ifdef DEBUG
		cout<<"remoteaddr is "<<remoteaddr<<endl;
		cout<<"requestparam is "<<requestparam<<endl;
#endif	
		if (errcode == E_SUCCESS)
		{
			pthread_mutex_lock(&context_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&context_mutex);
		}
		else
			writeLog(g_logid_local, LOGERROR, "request invalid,err:0x%x", errcode);

fcgi_finish:
		FCGX_Finish_r(&request);
	}
}

int main(int argc, char *argv[])
{
	char *ipbpath = NULL;
	int errcode = E_SUCCESS;
	vector<pthread_t> thread_id;	//每创建一个线程就将线程放在数组中
	
	string global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	string pv_conf = string(GLOBAL_PATH) + string(MONITORCONF);
	
	char *sz_global_conf = (char *)global_conf.c_str();
	char *sz_pv_conf = (char *)pv_conf.c_str();
	
	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);
	
	FCGX_Init();
	
	cpu_count = GetPrivateProfileInt(sz_global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		va_cout("read profile file failed,err:0x%x", errcode);
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
#ifdef DEBUG
	cout<<"cpu_count: "<<(int)cpu_count<<endl;
#endif
	ipbpath = GetPrivateProfileString(sz_global_conf, "default", "ipb");
	if(ipbpath == NULL)
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		errcode = E_FILE_IPB;
		goto exit;
	}
	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (id == NULL)
	{
		errcode = E_MALLOC;
		va_cout("id malloc failed,err:0x%x", errcode);
		goto exit;
	}
	pv_ip = GetPrivateProfileString(sz_pv_conf, "logserver", "pv_ip");
	pv_port = GetPrivateProfileInt(sz_pv_conf, "logserver", "pv_port");
	
	if ((pv_ip == NULL) || (pv_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
#ifdef DEBUG
	cout<<"pv_ip: "<<pv_ip<<endl;
	cout<<"pv_port: "<<pv_port<<endl;
#endif

	g_logid_local = openLog(sz_pv_conf, "locallog", true, &run_flag, thread_id);
	g_logid = openLog(sz_pv_conf, "log", true, &run_flag, thread_id);
	if((g_logid_local == 0) || (g_logid == 0))
	{
		errcode = E_INVALID_PARAM_LOGID;
		va_cout("openlog pv failed,err:0x%x", errcode);
		goto exit;
	}
	
	make_success_message();
	
	datatransfer = new DATATRANSFER;
	datatransfer->logid = g_logid_local;
	datatransfer->server = string(pv_ip);
	datatransfer->port = pv_port;
	datatransfer->run_flag = &run_flag;	//datatransfer的run_flag在最开始为true,线程运行的标识
	
connect_log_server:
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
#ifdef DEBUG
	cout<<"datatransfer->fd: "<<datatransfer->fd<<endl;
#endif
	if (datatransfer->fd == -1)
	{
		writeLog(g_logid_local, LOGERROR, "Connect pv server " + datatransfer->server + " failed!");
	}
	
	pthread_mutex_init(&datatransfer->mutex_data, 0);
	pthread_create(&datatransfer->threadlog, NULL, threadSendLog, datatransfer);	
	thread_id.push_back(datatransfer->threadlog);		//将发送日志的线程放在数组中
	
	for (uint64_t i = 0; i < cpu_count; i++)
		pthread_create(&id[i], NULL, doit, NULL);
	for (uint8_t i = 0 ; i < thread_id.size(); ++i)
	{
	   pthread_join(thread_id[i], NULL);
	}
exit:

	if (geodb != 0)
	{
		closeIPB(geodb);
	}

	FREE(pv_ip);
	FREE(ipbpath);
	
	if(g_logid_local != 0)
		closeLog(g_logid_local);

	if(g_logid != 0)
		closeLog(g_logid);

	if(datatransfer->fd != -1)
	{
		disconnectServer(datatransfer->fd);
	}

	pthread_mutex_destroy(&datatransfer->mutex_data);
	if(datatransfer != NULL)
	{
		delete datatransfer;
		datatransfer = NULL;
	}
	
	if(errcode == E_SUCCESS)			//执行成功时,需要退出杀掉所有线程
	{
		for (int i = 0;i < cpu_count; i++)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}
	
	if (id != NULL)
	{
		free(id);
		id = NULL;
	}
	
	va_cout("pv run error,err:0x%x", errcode);
	cout << "exit,errcode: " <<hex<<errcode << endl;

	return errcode;
	
}
