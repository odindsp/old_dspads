#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <queue>
#include <sstream>  //std::stringstream
#include "tracker.h"

using namespace std;

#define VERSION_TRACKER			"2.002"

#define	TRACKER_CONF_FILE	"dsp_tracker.conf"
// initialize_conf_price 中加载 string(GLOBAL_PATH) + string(PRICE_CONF_FILE);
#define	PRICE_CONF_FILE		"dsp_price.conf"

bool run_flag = true;
double price_ceiling = 0.00;
uint8_t cpu_count = 0;
uint64_t g_ctx = 0;
uint64_t g_logid_flume = 0, g_logid_kafka = 0;
uint64_t g_logid_local = 0;

DATATRANSFER *g_datatransfer_k = NULL; //kafka
DATATRANSFER *g_datatransfer_f = NULL; //flume
KAFKA_CONF kafka_conf;

pthread_t *pid;
pthread_mutex_t g_mutex_id, g_mutex_ur, g_mutex_group;
vector<string> cmds_id;
vector<GROUP_FREQUENCY_INFO> v_group_frequency;
vector<USER_INFO> v_user_frequency;
vector<bool> clk_redirect_status;	//点击重定向状态，执行302跳转后该状态置ture

char *master_group_ip = NULL, *slave_group_ip = NULL, *master_ur_ip = NULL, *slave_ur_ip = NULL;
uint16_t  master_group_port = 0, slave_group_port = 0, master_ur_port = 0, slave_ur_port = 0;

void* frequency_process(void *arg)
{
	uint64_t logid = *(uint64_t *)arg;
	redisContext *ctx_gr_r = NULL;
	redisContext *ctx_gr_w = NULL;
	
	ctx_gr_r = redisConnect(slave_group_ip, slave_group_port);
	if (ctx_gr_r == NULL)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process read connnect is NULL");
	}
	else if (ctx_gr_r->err)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process read failed,err:%s", ctx_gr_r->errstr);
		redisFree(ctx_gr_r);
		ctx_gr_r = NULL;
	}

	ctx_gr_w = redisConnect(master_group_ip, master_group_port);
	if (ctx_gr_w == NULL)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process write connnect is NULL");
	}
	else if (ctx_gr_w->err)
	{
		cflog(logid, LOGDEBUG, "connect frequency_process write failed,err:%s", ctx_gr_w->errstr);
		redisFree(ctx_gr_w);
		ctx_gr_w = NULL;
	}

	do 
	{
	//	usleep(PIPETIME);
		pthread_mutex_lock(&g_mutex_group);
		long fre_size = v_group_frequency.size();
		if(fre_size > 0)
		{
			vector<GROUP_FREQUENCY_INFO> v_group_frequency_temp;
			v_group_frequency_temp.swap(v_group_frequency);
			pthread_mutex_unlock(&g_mutex_group);

			struct timespec ts1, ts2;
			long lasttime = 0;
			getTime(&ts1);
			for(int i = 0; i < fre_size; ++i)
			{
				int errcode = E_SUCCESS;
				if (ctx_gr_r == NULL)
				{
					ctx_gr_r = redisConnect(slave_group_ip, slave_group_port);
					if (ctx_gr_r == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process read connnect is NULL");
					}
					else if (ctx_gr_r->err)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process read failed,err:%s", ctx_gr_r->errstr);
						redisFree(ctx_gr_r);
						ctx_gr_r = NULL;
					}
				}

				if (ctx_gr_w == NULL)
				{
					ctx_gr_w = redisConnect(master_group_ip, master_group_port);
					if (ctx_gr_w == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process write connnect is NULL");
					}
					else if (ctx_gr_w->err)
					{
						cflog(logid, LOGDEBUG, "reconnect frequency_process write failed,err:%s", ctx_gr_w->errstr);
						redisFree(ctx_gr_w);
						ctx_gr_w = NULL;
					}
				}

				if (ctx_gr_r != NULL && ctx_gr_w != NULL)
				{
					errcode = check_group_frequency_capping(ctx_gr_r, ctx_gr_w, &v_group_frequency_temp[i]);
					if (errcode == E_REDIS_CONNECT_R_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping read redisContext is invaild");
						redisFree(ctx_gr_r);
						ctx_gr_r = NULL;
					}
					else if (errcode == E_REDIS_CONNECT_W_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping write redisContext is invaild");
						redisFree(ctx_gr_w);
						ctx_gr_w = NULL;
					}
				}
			}

			getTime(&ts2);
			lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;	
			cflog(logid, LOGDEBUG, "frequency_process size:%ld,spent time:%ld", fre_size, lasttime);
		}
		else
		{
			pthread_mutex_unlock(&g_mutex_group);
			usleep(PIPETIME);
		}

	} while (run_flag);

exit:
	if (ctx_gr_r != NULL)
		redisFree(ctx_gr_r);
	if (ctx_gr_w != NULL)
		redisFree(ctx_gr_w);
	cout<<"frequency_process exit"<<endl;
}

void* read_write_ur(void *arg)
{
	uint64_t logid = *(uint64_t *)arg;

	redisContext *ctx_ur_r = NULL;
	redisContext *ctx_ur_w = NULL;

	ctx_ur_r = redisConnect(slave_ur_ip, slave_ur_port);
	if (ctx_ur_r == NULL)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur read connnect is NULL");
	}
	else if (ctx_ur_r->err)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur read failed,err:%s", ctx_ur_r->errstr);

		redisFree(ctx_ur_r);
		ctx_ur_r = NULL;
	}

	ctx_ur_w = redisConnect(master_ur_ip, master_ur_port);
	if (ctx_ur_w == NULL)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur write connnect is NULL");
	}
	else if (ctx_ur_w->err)
	{
		cflog(logid, LOGDEBUG, "connect read_write_ur write failed,err:%s", ctx_ur_w->errstr);

		redisFree(ctx_ur_w);
		ctx_ur_w = NULL;
	}

	do 
	{
	//	usleep(PIPETIME);
		pthread_mutex_lock(&g_mutex_ur);
		long fre_size = v_user_frequency.size();
		if(fre_size > 0)
		{
			vector<USER_INFO> v_user_frequency_temp;
			v_user_frequency_temp.swap(v_user_frequency);
			pthread_mutex_unlock(&g_mutex_ur);

			struct timespec ts1, ts2;
			long lasttime = 0;
			getTime(&ts1);
			for(int i = 0; i < fre_size; ++i)
			{
				int errcode = E_SUCCESS;
				if (ctx_ur_r == NULL)
				{
				   ctx_ur_r = redisConnect(slave_ur_ip, slave_ur_port);
					if (ctx_ur_r == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur read connnect is NULL");
					}
					else if (ctx_ur_r->err)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur read failed,err:%s", ctx_ur_r->errstr);

						redisFree(ctx_ur_r);
						ctx_ur_r = NULL;
					}
				}

				if (ctx_ur_w == NULL)
				{
				   ctx_ur_w = redisConnect(master_ur_ip, master_ur_port);
					if (ctx_ur_w == NULL)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur write connnect is NULL");
					}
					else if (ctx_ur_w->err)
					{
						cflog(logid, LOGDEBUG, "reconnect read_write_ur write failed,err:%s", ctx_ur_w->errstr);

						redisFree(ctx_ur_w);
						ctx_ur_w = NULL;
					}
				}

				if (ctx_ur_r != NULL && ctx_ur_w != NULL)
				{
				    errcode = UserRestrictionCount(ctx_ur_r, ctx_ur_w, logid, &v_user_frequency_temp[i]);
				    if (errcode == E_REDIS_CONNECT_R_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping read redisContext is invaild");
						redisFree(ctx_ur_r);
						ctx_ur_r = NULL;
					}
					else if (errcode == E_REDIS_CONNECT_W_INVALID)  // reconnect
					{
						cflog(logid, LOGDEBUG, "check_group_frequency_capping write redisContext is invaild");
						redisFree(ctx_ur_w);
						ctx_ur_w = NULL;
					}
				}
			}

			getTime(&ts2);
			lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;	
			cflog(logid, LOGDEBUG, "read_write_ur size:%ld,spent time:%ld", fre_size, lasttime);
		}
		else
		{
			pthread_mutex_unlock(&g_mutex_ur);
			usleep(PIPETIME);
		}

	} while (run_flag);

exit:
	if (ctx_ur_r != NULL)
		redisFree(ctx_ur_r);
	if (ctx_ur_w != NULL)
		redisFree(ctx_ur_w);
	cout<<"read_write_ur exit"<<endl;
}

static void *doit(void *arg)
{
	uint8_t index = (uint64_t)arg;
	int rc = 0;
	int errcode = E_SUCCESS;
	FCGX_Request request;

	char *remoteaddr = NULL, *requestparam = NULL;
	string send_data;
	string flume_data = "";
	string flume_err_data = "";
	string flume_base_data = "";

	stringstream ss;
	string errcode_str = "";


	//1. Load handle
	DATATRANSFER *datatransfer_f = &g_datatransfer_f[index/2];
	DATATRANSFER *datatransfer_k = &g_datatransfer_k[index/2];
	
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)g_ctx;
	if (pctx == NULL)
	{
		writeLog(g_logid_local, LOGERROR, "pctx is null, err:[0xE0093007], in:[%s], on:[%d]", __FUNCTION__, __LINE__);
		goto exit;
	}

	//2. Load 200 response data
	//send_data = pctx->messagedata;
	send_data = "Status: 200 OK\r\n                         \
					     Content-Type:text/plain; charset=utf-8\r\n \
					     Connection: close\r\n\r\n";
	static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
	static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
	//3. fast-cgi init request
	FCGX_InitRequest(&request, 0, 0);

	while (run_flag)
	{
		errcode = E_SUCCESS;

		//4. Accept the request message
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		cout << "rc:[" << rc << "]" << endl;
		writeLog(g_logid_local, LOGINFO,"rc:%d", rc);
		if (rc < 0)
		{
			errcode = E_TRACK_FAILED_FCGX_ACCEPT_R;
			flume_err_data = "emsg=FCGX_Accept_r failed";
			writeLog(g_logid_local, LOGERROR, "FCGX_Accept_r failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
			goto fcgi_finish;
		}

		remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
		requestparam = FCGX_GetParam("QUERY_STRING", request.envp);
		cout << "remoteaddr is:[" << remoteaddr << "]" << endl;
		cout << "requestparam is:[" << requestparam << "]" << endl;

		if ((remoteaddr != NULL) && (requestparam != NULL))
		{
			writeLog(g_logid_local, LOGINFO, string(remoteaddr) + " " + string(requestparam));
			// tracker/tracker.cpp
			errcode = parseRequest(request, g_ctx, index, datatransfer_f, datatransfer_k, remoteaddr, requestparam);
			if (errcode == E_SUCCESS)
			{
				flume_err_data = "emsg=parseRequest success";
				flume_base_data = "remoteaddr=" + string(remoteaddr) + "|requestparam=" + string(requestparam);
				// for test,  TODO: pctx->messagedata or text ???
				if (clk_redirect_status[index])
				{
					clk_redirect_status[index] = false;
				}
				else 
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(send_data.data(), send_data.size(), request.out);
					pthread_mutex_unlock(&context_mutex);
				}
				// 发送 flume 原始数据
				errcode_str = "|ecode=0x0";
				flume_data = make_error_message(flume_base_data, flume_err_data, errcode_str);
				if (flume_data != "")
				{
					sendLog(datatransfer_f, flume_data);
				}
			}
			else
			{
				writeLog(g_logid_local, LOGERROR, "The parseRequest invalid, err:[0x%x], remoteaddr:[%s], requestparam:[%s], in:[%s], on:[%d]", errcode, remoteaddr, requestparam, __FUNCTION__, __LINE__);
				// 发送 flume 原始数据
				ss << std::hex << errcode;
				ss >> errcode_str;
				ss.clear();
				ss.str("");
				errcode_str = "|ecode=0x" + errcode_str;
				flume_err_data = "emsg=parseRequest failed";
                                flume_base_data = "remoteaddr=" + string(remoteaddr) + "|requestparam=" + string(requestparam);
				flume_data = make_error_message(flume_base_data, flume_err_data, errcode_str);
                                if (flume_data != "")
                                {
                                        sendLog(datatransfer_f, flume_data);
                                }
				goto fcgi_finish;
			}
		}
		else if (remoteaddr == NULL)
		{
			errcode = E_TRACK_EMPTY_REMOTEADDR;
			writeLog(g_logid_local, LOGERROR, "The remoteaddr is NULL, err:[0x%x], remoteaddr:[%s], requestparam:[%s], in:[%s], on:[%d]", errcode, remoteaddr, requestparam, __FUNCTION__, __LINE__);
			goto fcgi_finish;
		}
		else if (requestparam == NULL)
		{
			errcode = E_TRACK_EMPTY_REQUESTPARAM;
			writeLog(g_logid_local, LOGERROR, "The requestparam is NULL, err:[0x%x], remoteaddr:[%s], requestparam:[%s], in:[%s], on:[%d]", errcode, remoteaddr, requestparam, __FUNCTION__, __LINE__);
			goto fcgi_finish;
		}

fcgi_finish:
		FCGX_Finish_r(&request);
	} // end of while
exit:
	cout<<"doit exit"<<endl;
	return NULL;
}

void sigroutine(int dunno)
{
	switch (dunno)
	{
		case SIGINT://SIGINT
		case SIGTERM://SIGTERM
			run_flag = false;
			break;
		default:
			break;
	}
}

int main(int argc, char *argv[])
{
	int errcode = E_SUCCESS;	
	int send_count = 0;
	char *price_upper = NULL;
	char* val = NULL;

	pthread_t thread_id, thread_ur, thread_group;
	int err_id = E_UNKNOWN, err_ur = E_UNKNOWN, err_group = E_UNKNOWN;
	PIPELINE_INFO wr_id;
	vector<pthread_t> t_thread;
	
	char *flume_ip = NULL;
	uint16_t flume_port = 0;
	char *master_filter_id_ip = NULL;
	uint16_t master_filter_id_port = 0;

	stringstream ss_price;

	//1. 获取配置文件地址
	string global_conf_str = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	char *global_conf = (char*)global_conf_str.c_str();
	string tracker_conf_str = string(GLOBAL_PATH) + string(TRACKER_CONF_FILE);
	char *tracker_conf = (char*)tracker_conf_str.c_str(); 
	cout << "global_conf_str:[" << global_conf_str << "]" <<  endl;   //test
	cout << "tracker_conf_str:[" << tracker_conf_str << "]" <<  endl; //test

	//2. init sinal for exit
	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	//3. init fast-cgi obj
	FCGX_Init();

	//4. 初始化本地日志服务的连接句柄
	g_logid_local = openLog(tracker_conf, "locallog", true, &run_flag, t_thread);
	if (g_logid_local == 0)
	{
		errcode = E_TRACK_INVALID_LOCAL_LOGID;
		cout << "Local logid's openlog failed, err:[0x" << std::hex << errcode 
		<< "], in:[" << __FUNCTION__ << "], on:[" << __LINE__ << "]" << endl;
		goto exit;
	}
	g_logid_flume = openLog(tracker_conf, "flumelog", true, &run_flag, t_thread);
	if (g_logid_flume == 0)
	{
		errcode = E_TRACK_INVALID_FLUME_LOGID;
		writeLog(g_logid_local, LOGERROR, "Flume logid's openlog failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	g_logid_kafka = openLog(tracker_conf, "kafkalog", true, &run_flag, t_thread);
	if (g_logid_kafka == 0)
	{
		errcode = E_TRACK_INVALID_KAFKA_LOGID;
		writeLog(g_logid_local, LOGERROR, "Kafka logid's openlog failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	cout << "step4 ok" << endl; // test
	
	//5. 获取参数 cpu_count、pid、price_ceiling
	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		errcode = E_TRACK_EMPTY_CPU_COUNT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	pid = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (pid == NULL)
	{
		errcode = E_TRACK_MALLOC_PID;
		writeLog(g_logid_local, LOGERROR, "Pid malloc failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	price_upper = GetPrivateProfileString(global_conf, "price", "upper");
	if (price_upper == NULL)
	{
		errcode = E_TRACK_EMPTY_PRICE_CEILING;
		writeLog(g_logid_local, LOGWARNING, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		// 默认值为 50(人民币元/CPM)
		price_ceiling = 50.00;
	}
	else
	{
		ss_price << price_upper;
		ss_price >> price_ceiling;
		ss_price.clear();
		ss_price.str("");
		if (price_ceiling < 50.00)
		{
			errcode = E_TRACK_UNDEFINE_PRICE_CEILING;
			writeLog(g_logid_local, LOGWARNING, "Read profile file's parametre is undefined, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
			price_ceiling = 50.00;
		}
	}
        cout << "step5 ok" << endl; // test	

	//6. 初始化 kafka 和 flume 的服务器信息集合 
	// common/util.h  DATATRANSFER 是日志服务器相关信息集合
	send_count = cpu_count/2;	
	g_datatransfer_k = new DATATRANSFER[send_count + 1];
	if (g_datatransfer_k == NULL)
	{
		errcode = E_TRACK_MALLOC_DATATRANSFER_K;
		writeLog(g_logid_local, LOGERROR, "The g_datatransfer_k malloc failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	g_datatransfer_f = new DATATRANSFER[send_count + 1];
	if (g_datatransfer_f == NULL)
	{
		errcode = E_TRACK_MALLOC_DATATRANSFER_F;
		writeLog(g_logid_local, LOGERROR, "The g_datatransfer_f malloc failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
        cout << "step6 ok" << endl; // test     

	//7. 获取 redis 地址
	master_ur_ip = GetPrivateProfileString(tracker_conf, (char*)"redis", (char*)"master_ur_ip");
	master_ur_port = GetPrivateProfileInt(tracker_conf, (char*)"redis", (char*)"master_ur_port");
	if (master_ur_ip == NULL)
	{
		errcode = E_TRACK_EMPTY_MASTER_UR_IP;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (master_ur_port == 0)
	{
		errcode = E_TRACK_EMPTY_MASTER_UR_PORT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	slave_ur_ip = GetPrivateProfileString(tracker_conf, (char*)"redis", (char*)"slave_ur_ip");
	slave_ur_port = GetPrivateProfileInt(tracker_conf, (char*)"redis", (char*)"slave_ur_port");
	if (slave_ur_ip == NULL)
	{
		errcode = E_TRACK_EMPTY_SLAVE_UR_IP;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (slave_ur_port == 0)
	{
		errcode = E_TRACK_EMPTY_SLAVE_UR_PORT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	master_filter_id_ip = GetPrivateProfileString(tracker_conf, (char*)"redis", (char*)"master_filter_id_ip");
	master_filter_id_port = GetPrivateProfileInt(tracker_conf, (char*)"redis", (char*)"master_filter_id_port");
	if (master_filter_id_ip == NULL)
	{
		errcode = E_TRACK_EMPTY_MASTER_FILTER_ID_IP;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (master_filter_id_port == 0)
	{
		errcode = E_TRACK_EMPTY_MASTER_FILTER_ID_PORT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	master_group_ip = GetPrivateProfileString(tracker_conf, (char*)"redis", (char*)"master_gr_ip");
	master_group_port = GetPrivateProfileInt(tracker_conf, (char*)"redis", (char*)"master_gr_port");
	if (master_group_ip == NULL)
	{
		errcode = E_TRACK_EMPTY_MASTER_GROUP_IP;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (master_group_port == 0)
	{
		errcode = E_TRACK_EMPTY_MASTER_GROUP_PORT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	slave_group_ip = GetPrivateProfileString(tracker_conf, (char*)"redis", (char*)"slave_gr_ip");
	slave_group_port = GetPrivateProfileInt(tracker_conf, (char*)"redis", (char*)"slave_gr_port");
	if (slave_group_ip == NULL)
	{
		errcode = E_TRACK_EMPTY_SLAVE_GROUP_IP;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (slave_group_port == 0)
	{
		errcode = E_TRACK_EMPTY_SLAVE_GROUP_PORT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	cout << "step7 ok" << endl; // test     
	
	//8. 初始化监视器
	errcode = global_initialize(g_logid_local, cpu_count, &run_flag, tracker_conf, t_thread, &g_ctx);
	if ((errcode != E_SUCCESS))
	{
		writeLog(g_logid_local, LOGERROR, "The global_initialize failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	cout << "step8 ok" << endl; // test     

	//9. 初始化各 redis 句柄互斥锁
	err_id = pthread_mutex_init(&g_mutex_id, 0);
	if (err_id != E_SUCCESS)
	{
		errcode = E_TRACK_INVALID_MUTEX_IDFLAG;
		writeLog(g_logid_local, LOGERROR, "Init mutex lock failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	err_ur = pthread_mutex_init(&g_mutex_ur, 0);
	if (err_ur != E_SUCCESS)
	{
		errcode = E_TRACK_INVALID_MUTEX_USER;
		writeLog(g_logid_local, LOGERROR, "Init mutex lock failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	err_group = pthread_mutex_init(&g_mutex_group, 0);
	if (err_group != E_SUCCESS)
	{
		errcode = E_TRACK_INVALID_MUTEX_GROUP;
		writeLog(g_logid_local, LOGERROR, "Init mutex lock failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	cout << "step9 ok" << endl; // test     

	//10. 启动去重线程
	init_pipeinfo(g_logid_local,"redis_write_id",master_filter_id_ip,master_filter_id_port,&cmds_id,&g_mutex_id,&run_flag,&wr_id);
	errcode = pthread_create(&thread_id, NULL, threadpipeline, &wr_id);
	if (errcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "Create thread threadpipeline failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	t_thread.push_back(thread_id);

	errcode = pthread_create(&thread_ur, NULL, read_write_ur, &g_logid_local);
	if (errcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "Create thread read_write_ur failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	t_thread.push_back(thread_ur);

	errcode = pthread_create(&thread_group, NULL, frequency_process, &g_logid_local);
	if (errcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "Create thread frequency_process failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	t_thread.push_back(thread_group);
	cout << "step10 ok" << endl; // test     

	//11. 初始化连接 flume server 和 kafka server 的句柄
	// 获取 flume server 地址
	// 测试时 flume server 用 python 模拟 (127.0.0.1:9093)
	flume_ip = GetPrivateProfileString(global_conf, "flumeserver", "flume_ip");
	flume_port = GetPrivateProfileInt(global_conf, "flumeserver", "flume_port");
	if (flume_ip == NULL)
	{
		errcode = E_TRACK_EMPTY_FLUME_IP;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (flume_port == 0)
	{
		errcode = E_TRACK_EMPTY_FLUME_PORT;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	// 获取 kafka server 地址
	// ../common/rdkafka_operator.h  KAFKA_CONF
	// 测试地址 (127.0.0.1:9092)
	init_topic_struct(&kafka_conf);
	val = GetPrivateProfileString(global_conf, "kafkaserver", "topic");
	if (val == NULL)
	{
		errcode = E_TRACK_EMPTY_KAFKA_TOPIC;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	kafka_conf.topic = val;
	free(val);
	val = GetPrivateProfileString(global_conf, "kafkaserver", "broker_list");
	if (val == NULL)
	{
		errcode = E_TRACK_EMPTY_KAFKA_BROKER_LIST;
		writeLog(g_logid_local, LOGERROR, "Read profile file failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	kafka_conf.broker_list = val;
	free(val);
	// 连接 flume server
	for (int i = 0 ; i <= (cpu_count/2); i++)
	{
		// connect flume server
		connect_log_server(g_logid_flume, flume_ip, flume_port, &run_flag, &g_datatransfer_f[i]);
		t_thread.push_back(g_datatransfer_f[i].threadlog);
	}
	// 连接 kafka server
	for (int i = 0; i <= (cpu_count/2); i++)
	{
		// connect kafka server
		connect_log_server(g_logid_kafka, &kafka_conf, &run_flag, &g_datatransfer_k[i]);
		t_thread.push_back(g_datatransfer_k[i].threadlog);
	}
	cout << "step11 ok" << endl; // test     

	//12. 启动解析request线程 
	for (uint64_t i = 0; i < cpu_count; ++i)
	{
		clk_redirect_status.push_back(false);
		pthread_create(&pid[i], NULL, doit, (void *)i);
	}
	cout << "step12 ok" << endl; // test     

	//13. 等待所有线程结束
	for (uint8_t i = 0 ; i < t_thread.size(); ++i)
	{
		pthread_join(t_thread[i], NULL);
	}

exit:
	//14. 释放占用资源
	if (err_id == E_SUCCESS)
	{
		pthread_mutex_destroy(&g_mutex_id);
	}
	if (err_ur == E_SUCCESS)
	{
		pthread_mutex_destroy(&g_mutex_ur);
	}
	if (err_group == E_SUCCESS)
	{
		pthread_mutex_destroy(&g_mutex_group);
	}

	IFFREENULL(flume_ip);
	IFFREENULL(master_ur_ip);
	IFFREENULL(slave_ur_ip);
	IFFREENULL(master_filter_id_ip);
	IFFREENULL(master_group_ip);
	IFFREENULL(slave_group_ip);

	if(g_logid_local)
	{
		closeLog(g_logid_local);
	}
	if (g_logid_flume != 0)
	{   
		closeLog(g_logid_flume);
	}
	if (g_logid_kafka != 0)
	{
		closeLog(g_logid_kafka);
	}

	if (g_ctx != 0)
	{
		int errcode = global_uninitialize(g_ctx);
		g_ctx = 0;
	}

	if (g_datatransfer_f != NULL)
	{
		for (uint8_t i = 0; i <= cpu_count / 2; ++i)
		{
			// disconnect flume server
			disconnect_log_server(&g_datatransfer_f[i]);
		}
		delete []g_datatransfer_f;
		g_datatransfer_f = NULL;
	}
	if (g_datatransfer_k != NULL)
	{
		for (uint8_t i = 0; i <= cpu_count / 2; ++i)
		{
			// disconnect kafka server
			// ../common/util.h
			disconnect_log_server(&g_datatransfer_k[i], true);
		}
		delete []g_datatransfer_k;
		g_datatransfer_k = NULL;
	}

	if (errcode == E_SUCCESS)
	{
		for(uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(pid[i], SIGKILL);
		}
	}

	IFFREENULL(pid);

	cout << "odin tracker exit,errcode:0x" << hex << errcode << endl;
	return errcode;
}
