#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "baiduvideos_response.h"
#include "../../common/errorcode.h"
#include "../../common/bid_filter.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF		"dsp_baiduvideos.conf"
#define DEVMAKETABLE 		"make_baiduvideos.txt"


uint64_t g_logid, g_logid_local, g_logid_response;
uint64_t g_ctx = 0;
uint8_t cpu_count = 0;
pthread_t *id = NULL;
int *numcount = NULL;
uint64_t geodb = 0;
bool fullreqrecord = false;
map<string, uint16_t> dev_make_table;
MD5_CTX hash_ctx;
bool run_flag = true;


static void *doit(void *arg)
{
	int errcode = 0;
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	int rc = 0;
	FLOW_BID_INFO flow_bid(ADX_BAIDU_VIDEOS);

	//�յ�������
	RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
	if (recvdata == NULL)
	{
		cflog(g_logid_local, LOGERROR, "recvdata:alloc memory failed!");
		goto exit;
	}

	//��ʼ������
	recvdata->length = 0;
	recvdata->buffer_length = MAX_REQUEST_LENGTH;
	recvdata->data = (char *)malloc(recvdata->buffer_length);
	if (recvdata->data == NULL)
	{
		cflog(g_logid_local, LOGERROR, "recvdata->data: malloc memory failed!");
		goto exit;
	}

	FCGX_InitRequest(&request, 0, 0);

	while (run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

		char *remoteaddr = NULL;
		char *method = NULL;
		char *contenttype = NULL;
		uint32_t contentlength = 0;
		string senddata;

		//����REQUEST
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);
		if (rc < 0)
		{
			cflog(g_logid_local, LOGERROR, "FCGX_Accept_r return!, rc:%d", rc);
			break;
		}

		flow_bid.reset();
		//��ȡREQUEST��ַ
		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		if (remoteaddr == NULL)
		{
			cflog(g_logid_local, LOGERROR, "not find REMOTE_ADDR");
			flow_bid.set_err(E_REQUEST_HTTP_REMOTE_ADDR);
			goto finish;
		}

		//��ȡREQUEST�ύ��ʽ
		method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		if (method == NULL)
		{
			cflog(g_logid_local, LOGERROR, "not find REQUEST_METHOD");
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
			goto finish;
		}

		if (strcmp(method, "POST") != 0)
		{
			char *requestparam = FCGX_GetParam("QUERY_STRING", request.envp);
			if (requestparam != NULL && strstr(requestparam, "pxene_debug=1") != NULL)
			{
				string data;
				get_server_content(g_ctx, index, data);
				pthread_mutex_lock(&counts_mutex);
				FCGX_PutStr(data.data(), data.length(), request.out);
				pthread_mutex_unlock(&counts_mutex);
			}

			cflog(g_logid_local, LOGERROR, "REQUEST_METHOD:%s is not POST.", method);
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
			goto finish;
		}

		//��ȡREQUEST��HTTPͷ����
		contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
		if (contenttype == NULL)
		{
			cflog(g_logid_local, LOGERROR, "not find CONTENT_TYPE");
			flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
			goto finish;
		}

		if (strcasecmp(contenttype, "application/json"))
		{
			cflog(g_logid_local, LOGERROR, "Wrong Content-Type: %s", contenttype);
			flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
			goto finish;
		}

		//��ȡREQUEST�ĳ���
		contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
		va_cout("contentlength: %d", contentlength);
		if (contentlength == 0)
		{
			cflog(g_logid_local, LOGERROR, "not find CONTENT_LENGTH or is 0");
			flow_bid.set_err(E_REQUEST_HTTP_CONTENT_LEN);
			goto finish;
		}

		//REQUEST���ȴ�����󳤶�
		if (contentlength >= recvdata->buffer_length)
		{
			cflog(g_logid_local, LOGERROR, "CONTENT_LENGTH is too big!!!!!!! contentlength:%d! realloc recvdata.", contentlength);
			recvdata->buffer_length = contentlength + 1;
			recvdata->data = (char *)realloc(recvdata->data, recvdata->buffer_length);

			if (recvdata->data == NULL)
			{
				recvdata->buffer_length = 0;
				cflog(g_logid_local, LOGERROR, "recvdata->data: realloc memory failed!");
				flow_bid.set_err(E_MALLOC);
				goto finish;
			}
		}

		recvdata->length = contentlength;

		//��ȡREQUEST����recvdata��
		if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
		{
			cflog(g_logid_local, LOGERROR, "FCGX_GetStr failed!");
			flow_bid.set_err(E_REQUEST_HTTP_BODY);
			goto finish;
		}

		//��ӽ�����
		recvdata->data[recvdata->length] = 0;

		//�������fullreqrecordΪtrue,���¼REQUEST
		if (fullreqrecord)
		{
			writeLog(g_logid_local, LOGDEBUG, "recvdata= " + string(recvdata->data, recvdata->length));
		}

		//��ȡ����Ӧ�𷵻�ֵ
		//int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata,OUT string &senddata)
		errcode = get_bid_response(g_ctx, index, recvdata, senddata, flow_bid);

		//����������ȷ(errcodeΪE_BAD_REQUEST����E_INVALID_PARAM)����502,���򷵻�200(���۳ɹ�)����204(���۲��ɹ�)
		if (senddata != "")
		{
			pthread_mutex_lock(&counts_mutex);
			//��RESPONSE����
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&counts_mutex);
		}
		else if (errcode == E_REDIS_CONNECT_INVALID)		//���Ӳ���REDIS
		{
			syslog(LOG_INFO, "redis connect invalid");
			kill(getpid(), SIGTERM);		//��ֹ����
		}

	finish:
		g_ser_log.send_detail_log(index, flow_bid.to_string());
		FCGX_Finish_r(&request);
	}

exit:
	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);

	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
}

void sigroutine(int dunno)
{
	/* �źŴ������̣�����dunno����õ��źŵ�ֵ */

	switch (dunno)
	{
	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		break;
	default:
		cout << "Get a unknown -- " << dunno << endl;
		break;
	}
	return;
}

int main(int argc, char *argv[])
{
	int errcode = 0;
	bool is_textdata = false;
	char *ipbpath = NULL;
	bool is_print_time = false;
	map<string, uint16_t>::iterator it_make;	//�����̵�����

	//ȫ�������ļ�·����/etc/dspads_odin/dspads.conf
	string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char *)str_global_conf.c_str();

	//ADX�����ļ�·����/etc/dspads_odin/dsp_xxx.conf
	string str_private_conf = string(GLOBAL_PATH) + string(PRIVATE_CONF);
	char *private_conf = (char *)str_private_conf.c_str();
	vector<pthread_t> thread_id;

	//�����߳���
	cpu_count = GetPrivateProfileInt((char *)str_global_conf.c_str(), "default", "cpu_count");
	va_cout("cpu_count:%d", cpu_count);

	if (cpu_count < 1)
	{
		va_cout("cpu_count error!");
		errcode = E_INVALID_CPU_COUNT;
		run_flag = false;
		goto exit;
	}
	//ipb��·��
	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if (ipbpath == NULL)
	{
		errcode = E_INVALID_PROFILE_IPB;
		run_flag = false;
		goto exit;
	}

	//�߳���
	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (id == NULL)
	{
		errcode = E_MALLOC;
		run_flag = false;
		goto exit;
	}

	//ǧ�μ���,ÿǧ�δ�ӡһ��RESPONSE
	numcount = (int *)calloc(cpu_count, sizeof(int));
	if (numcount == NULL)
	{
		errcode = E_MALLOC;
		run_flag = false;
		goto exit;
	}

	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	FCGX_Init();

	//��IPB
	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		errcode = E_FILE_IPB;
		run_flag = false;
		goto exit;
	}

	//��ȡ��ӡLog��־��С
	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time  %d", is_print_time);

	//��ȡ���ش�ӡ����Ϊ�ı���ʽ
	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	va_cout("locallog is_textdata  %d", is_textdata);

	//�򿪱�����־
	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_local == 0)
	{
		va_cout("openLog failed! name:%s, section:%s", private_conf, "locallog");
		errcode = E_INVALID_PARAM_LOGID_LOC;
		run_flag = false;
		goto exit;
	}

	//��ȡ��־��ӡ�Ƿ�Ϊ�ı���ʽ
	is_textdata = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	va_cout("log is_textdata  %d", is_textdata);

	//����־
	g_logid = openLog(private_conf, "log", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid == 0)
	{
		cflog(g_logid_local, LOGERROR, "openLog failed! name:%s, section:%s", private_conf, "log");
		errcode = E_INVALID_PARAM_LOGID;
		run_flag = false;
		goto exit;
	}

	//��ȡ��ӡRESPONSE��־�Ƿ�Ϊ�ı���ʽ
	is_textdata = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");
	va_cout("logresponse is_textdata  %d", is_textdata);

	//��RESPONSE��־
	g_logid_response = openLog(private_conf, "logresponse", is_textdata, &run_flag, thread_id, is_print_time);
	if (g_logid_response == 0)
	{
		errcode = E_INVALID_PARAM_LOGID_RESP;
		run_flag = false;
		goto exit;
	}

	//��ȡ�����̱���ʼ��
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), 
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		cflog(g_logid_local, LOGERROR, "init make table failed!");
		errcode = E_PROFILE_INIT_MAKE;
		run_flag = false;
		goto exit;
	}

	for (it_make = dev_make_table.begin(); it_make != dev_make_table.end(); ++it_make)
	{
		cout << "dump make: " << it_make->first << " -> " << it_make->second << endl;
	}

	//�Ƿ��ӡ�յ�����
	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");
	va_cout("log fullreqrecord  %d", fullreqrecord);

	//��ʼ��MD5��ctx
	MD5_Init(&hash_ctx);

	//���۹��˳�ʼ��
	errcode = bid_filter_initialize(g_logid_local, ADX_BAIDU_VIDEOS, cpu_count, &run_flag, private_conf, thread_id, &g_ctx);
	if (errcode != E_SUCCESS)
	{
		cflog(g_logid_local, LOGERROR, "bid_filter_initialize failed! errcode:0x%x", errcode);
		run_flag = false;
		goto exit;
	}
	cflog(g_logid_local, LOGINFO, "bid_filter_initialize success! ctx:0x%x", g_ctx);

	errcode = g_ser_log.init(cpu_count, ADX_BAIDU_VIDEOS, g_logid_local, private_conf, &run_flag);
	if (errcode != E_SUCCESS)
	{
		FAIL_SHOW;
		run_flag = false;
		goto exit;
	}

	for (uint8_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}

exit:
	g_ser_log.uninit();
	if (g_logid_local)
		closeLog(g_logid_local);

	if (g_logid)
		closeLog(g_logid);

	if (g_logid_response)
		closeLog(g_logid_response);

	if (g_ctx != 0)
	{
		int errcode = bid_filter_uninitialize(g_ctx);
		cout << "main: bid_filter_uninitialize, errcode:0x" << hex << errcode << endl;
		g_ctx = 0;
	}

	if (geodb != 0)
	{
		closeIPB(geodb);
	}

	if (errcode == E_SUCCESS)
	{
		for (uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}

	if (id)
	{
		free(id);
		id = NULL;
	}

	cout << "main exit, errcode:0x" << hex << errcode << endl;
	return 0;
}
