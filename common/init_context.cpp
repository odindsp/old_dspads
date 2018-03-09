#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "init_context.h"
#include "setlog.h"
#include "errorcode.h"

extern "C"
{
#include <dlfcn.h>
	//  #include <ccl/ccl.h>
}

using namespace std;

void init_pipeinfo(uint64_t logid,string message, string redisip, uint16_t redisport, vector<string> *v_cmd, pthread_mutex_t *v_mutex, bool *run_flag, PIPELINE_INFO *info)
{
	info->logid = logid;
	info->message = message;
	info->redisip = redisip;
	info->redisport = redisport;
	info->v_cmd = v_cmd;
	info->v_mutex = v_mutex;
	info->run_flag = run_flag;
}

void* threadpipeline(void *arg)
{
	PIPELINE_INFO *info = (PIPELINE_INFO *)arg;
	redisContext *context = NULL;

	if (info == NULL)
	{
	  cout<<"threadpipeline info null"<<endl;
	  goto exit;
	}

	context = redisConnect(info->redisip.c_str(), info->redisport);
	if(context == NULL)
	{
		cflog(info->logid, LOGDEBUG, "connect %s is NULL", info->message.c_str());
	}
	else if(context->err)
	{
		cflog(info->logid, LOGDEBUG, "connect %s failed,err:%s", info->message.c_str(), context->errstr);
		redisFree(context);
		context = NULL;
	}

	do
	{
	    usleep(PIPETIME);
		struct timespec ts1, ts2;
		long lasttime = 0;

		getTime(&ts1);
		pthread_mutex_lock(info->v_mutex);
		long cmds_size = (*info->v_cmd).size();

		if (cmds_size > 0)
		{
			vector<string> cmds_temp;
			cmds_temp.swap(*info->v_cmd);

			pthread_mutex_unlock(info->v_mutex);
			if (context == NULL)
			{
				context = redisConnect(info->redisip.c_str(), info->redisport);
				if(context == NULL)
				{
					cflog(info->logid, LOGDEBUG, "reconnect %s is NULL", info->message.c_str());
					continue;
				}
				else if(context->err)
				{
				   cflog(info->logid, LOGDEBUG, "reconnect %s failed,err:%s", info->message.c_str(), context->errstr);
				   redisFree(context);
				   context = NULL;
				   continue;
				}
			}

			for(unsigned long i = 0; i < cmds_size; ++i)
				redisAppendCommand(context, cmds_temp[i].c_str());

			for(unsigned long i = 0; i < cmds_size; ++i)
			{
				redisReply *reply = NULL;

				redisGetReply(context, (void **)&reply);

				if (reply != NULL )
				{
					freeReplyObject(reply);
				}
				else   // reconnect
				{
					if (context != NULL)
					{
					   cflog(info->logid, LOGDEBUG, "redisGetReply %s failed,err:%s,commands is %s",info->message.c_str(), context->errstr,cmds_temp[i].c_str());
		               redisFree(context);
					   context = NULL;
					   break;
					}
				}
			}
			getTime(&ts2);
			lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

			cflog(info->logid, LOGDEBUG, "%s pipeline size:%ld,spent time:%ld", info->message.c_str(), cmds_size, lasttime);
		}
		else
		{
			pthread_mutex_unlock(info->v_mutex);	
		}
	}while(*info->run_flag);

exit:
	if (context != NULL)
		redisFree(context);
	cout<<info->message<<" exit"<<endl;
}

void *thread_read_aprice(void *arg)
{

	MONITORCONTEXT *pctx = (MONITORCONTEXT *)arg;

	redisContext *context_r = NULL, *context_w = NULL;

	if (pctx == NULL)
	{
		cout<<"thread_read_aprice info null"<<endl;
		goto exit;
	}

	context_r = redisConnect(pctx->slave_filter_ip.c_str(), pctx->slave_filter_port);
	if(context_r == NULL)
	{
		cflog(pctx->logid, LOGDEBUG, "connect thread_read_aprice read is NULL");
	}
	else if(context_r->err)
	{
		cflog(pctx->logid, LOGDEBUG, "connect thread_read_aprice read failed,err:%s", context_r->errstr);
		redisFree(context_r);
		context_r = NULL;
	}

	context_w = redisConnect(pctx->master_filter_ip.c_str(), pctx->master_filter_port);
	if(context_w == NULL)
	{
		cflog(pctx->logid, LOGDEBUG, "connect thread_read_aprice write is NULL");
	}
	else if(context_w->err)
	{
		cflog(pctx->logid, LOGDEBUG, "connect thread_read_aprice write failed,err:%s", context_w->errstr);
		redisFree(context_w);
		context_w = NULL;
	}

	do
	{
	    usleep(PIPETIME);
		struct timespec ts1, ts2;
		long lasttime = 0;

		getTime(&ts1);
		pthread_mutex_lock(&pctx->mutex_price_info);
		long cmds_ap_size = pctx->v_price_info.size();

		if (cmds_ap_size > 0)
		{
			vector<ADX_PRICE_INFO> cmds_ap_temp;
			cmds_ap_temp.swap(pctx->v_price_info);
			pthread_mutex_unlock(&pctx->mutex_price_info);

			for(unsigned long i = 0; i < cmds_ap_temp.size(); ++i)
			{
				if (context_r == NULL)
				{
					context_r = redisConnect(pctx->slave_filter_ip.c_str(), pctx->slave_filter_port);
					if(context_r == NULL)
					{
						cflog(pctx->logid, LOGDEBUG, "reconnect thread_read_aprice read is NULL");
						continue;
					}
					else if(context_r->err)
					{
						cflog(pctx->logid, LOGDEBUG, "reconnect thread_read_aprice read failed,err:%s", context_r->errstr);
						redisFree(context_r);
						context_r = NULL;
						continue;
					}
				}
				if (context_w == NULL)
				{
					context_w = redisConnect(pctx->master_filter_ip.c_str(), pctx->master_filter_port);
					if(context_w == NULL)
					{
						cflog(pctx->logid, LOGDEBUG, "reconnect thread_read_aprice write is NULL");
						continue;
					}
					else if(context_w->err)
					{
						cflog(pctx->logid, LOGDEBUG, "reconnect thread_read_aprice write failed,err:%s", context_w->errstr);
						redisFree(context_w);
						context_w = NULL;
						continue;
					}
				}
				if (context_r != NULL && context_w != NULL)
				{
					int errcode = E_SUCCESS;
				    errcode = set_average_price(context_r, context_w, pctx->logid, cmds_ap_temp[i]);
					if (errcode == E_REDIS_CONNECT_R_INVALID)  // reconnect
					{
						cflog(pctx->logid, LOGDEBUG, "thread_read_aprice read redisContext is invaild");
						redisFree(context_r);
						context_r = NULL;
					}
					else if (errcode == E_REDIS_CONNECT_W_INVALID)  // reconnect
					{
						cflog(pctx->logid, LOGDEBUG, "thread_read_aprice write redisContext is invaild");
						redisFree(context_w);
						context_w = NULL;
					}
				}
			}

			getTime(&ts2);
			lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

			cflog(pctx->logid, LOGDEBUG, "thread_read_aprice pipeline size:%ld,spent time:%ld", cmds_ap_size, lasttime);
		}
		else
		{
			pthread_mutex_unlock(&pctx->mutex_price_info);
		}

	}while(*(pctx->run_flag));

exit:
	if (context_r != NULL)
		redisFree(context_r);
	if (context_w != NULL)
		redisFree(context_w);
	cout<<"thread_read_aprice exit"<<endl;
}

int make_success_message(string &message)
{
 	char gif[64] = {
		0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0xff, 0x00, 0xff,
 		0x00, 0x00, 0x00, 0x21, 0xf9, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00,
 		0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3b, 0x00};
	
	char success_msg[4096] = "Status: 200 OK\r\nContent-Type: image/gif; charset=utf-8\r\nConnection: close\r\n\r\n";
	int success_msg_len = strlen(success_msg);

	memcpy(success_msg + success_msg_len, gif, 44);
	success_msg_len += 44;

	message = string(success_msg, success_msg_len);
	
	return 0;
}

int initialize_conf_price(vector<ADX_CONTENT> &adx_path)
{
		/* if we want to delete the platfrom, vector must be refresh, the decode function also changes at the same time */
		for(int i = 1; i < 256; ++i)
		{
			char *path = NULL;
			char index[10] = "";
			ADX_CONTENT adx_content;
			string conf_price = string(GLOBAL_PATH) + string(PRICECONF);
			
			sprintf(index, "%d", i);
			adx_content.name = 0;
			adx_content.dl_handle = NULL;
			
			path = GetPrivateProfileString((char *)conf_price.c_str(), (char *)"default", index);

			if (path != NULL)
			{
				adx_content.name = i;
				
				adx_content.dl_handle = dlopen(path, RTLD_LAZY);

				adx_path.push_back(adx_content);
			}
			else
				break;
		}

	return 0;
}

int set_average_price(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid, ADX_PRICE_INFO &price_info)
{
	string value = "";
	char *text = NULL;
	char szkeys[MIN_LENGTH] = "", now_time[MIN_LENGTH] = "";
	json_t *root = NULL, *label = NULL;
	struct timeval timeofday;
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);
	int errcode = E_SUCCESS;

	int adx = price_info.adx;
	string groupid = price_info.groupid;
	string mapid = price_info.mapid;
	double price = price_info.price;
	
	sprintf(now_time, "%02d", tm.tm_hour);
	sprintf(szkeys, "dsp_groupid_price_%02d_%02d_%s", adx, tm.tm_mday, groupid.c_str());
	errcode = hgetRedisValue(ctx_r, logid, string(szkeys), mapid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		errcode = E_REDIS_CONNECT_R_INVALID;
		goto resource_free;
	}

	if (value == "")
	{
		root = json_new_object();
		jsonInsertDouble(root, now_time, price);
		json_tree_to_string(root, &text);

		errcode = hsetRedisValue(ctx_w, logid, szkeys, mapid, string(text));
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			goto resource_free;
		}
		errcode = expireRedisValue(ctx_w, logid, szkeys, DAYSECOND*3 - getCurrentTimeSecond());
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			goto resource_free;
		}

		goto resource_free;
	}

	json_parse_document(&root, value.c_str());
	if (root == NULL)
		goto resource_free;

	if ((label = json_find_first_label(root, now_time)) != NULL)
	{
		double aver_price = atof(label->child->text);
		char *new_price = (char *)malloc(MIN_LENGTH * sizeof(char));

		memset(new_price, 0 , MIN_LENGTH * sizeof(char));
		aver_price = (aver_price + price) / 2;
		sprintf(new_price, "%f", aver_price);
		free(label->child->text);
		label->child->text = new_price;
	}
	else
		jsonInsertDouble(root, now_time, price);

exit:
	json_tree_to_string(root, &text);

	errcode = hsetRedisValue(ctx_w, logid, szkeys, mapid, string(text));
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
	}

resource_free:
	if (root != NULL)
		json_free_value(&root);

	if (text != NULL)
		free(text);

	return errcode;
}

int Decode(ADX_CONTENT &adx_path, char *encode_price, double &price)
{
	int errcode = E_SUCCESS;
	char *error = NULL;
	void *dl_handle = NULL;
	typedef int (*func)(char*, double*);

	if(adx_path.dl_handle != NULL)
	{
		func fc = (func)dlsym(adx_path.dl_handle, "DecodeWinningPrice");

		error = dlerror();
		if(error != NULL)
		{
			errcode = E_DL_GET_FUNCTION_FAIL;
			goto exit;
		}

		errcode = fc(encode_price, &price);
		if (errcode != E_SUCCESS)
		{
			errcode = E_DL_DECODE_FAILED;
			goto exit;
		}
	}
	else
	{
		errcode = E_DL_OPEN_FAIL;
		goto exit;
	}

exit:
	return errcode;
}

int global_initialize(IN uint64_t logid, IN uint8_t cpu_count, IN bool *run_flag, IN char *priv_conf, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx)
{
	int errcode = E_SUCCESS;

	uint64_t cache = 0;
	string global_conf = "";

	pthread_t thread_t; 

	char *slave_major_ip = NULL, *slave_filter_id_ip = NULL,  *pub_sub_ip = NULL, *master_filter_ip = NULL, *slave_filter_ip = NULL;
	uint16_t slave_major_port = 0, slave_filter_id_port = 0,  pub_sub_port = 0, master_filter_port = 0, slave_filter_port = 0;

	// 20170708: add for new tracker
	char *master_price_ip = NULL;
	uint16_t master_price_port = 0;

	MONITORCONTEXT *ctx;

	//global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	global_conf = priv_conf;

	
	if ((logid == 0) || (cpu_count == 0))
	{
		errcode = E_INVALID_PARAM;
		goto exit;
	}

	ctx = new MONITORCONTEXT;
	if (ctx == NULL)
	{
		errcode = E_MALLOC;
		goto exit;
	}

	ctx->logid = logid; 

	slave_major_ip = GetPrivateProfileString((char *)global_conf.c_str(), "redis", "slave_major_ip");
	slave_major_port = GetPrivateProfileInt((char *)global_conf.c_str(), "redis", "slave_major_port");

	slave_filter_id_ip = GetPrivateProfileString((char *)global_conf.c_str(), "redis", "slave_filter_id_ip");
	slave_filter_id_port = GetPrivateProfileInt((char *)global_conf.c_str(), "redis", "slave_filter_id_port");

//	slave_ur_ip = GetPrivateProfileString((char *)global_conf.c_str(), "redis", "slave_ur_ip");
//	slave_ur_port = GetPrivateProfileInt((char *)global_conf.c_str(), "redis", "slave_ur_port");

	pub_sub_ip = GetPrivateProfileString((char *)global_conf.c_str(), "redis", "pub_sub_ip");
	pub_sub_port = GetPrivateProfileInt((char *)global_conf.c_str(), "redis", "pub_sub_port");

	master_filter_ip = GetPrivateProfileString((char *)global_conf.c_str(), (char *)"redis", (char *)"master_filter_ip");
	master_filter_port = GetPrivateProfileInt((char *)global_conf.c_str(), (char *)"redis", (char *)"master_filter_port");

	slave_filter_ip = GetPrivateProfileString((char *)global_conf.c_str(), (char *)"redis", (char *)"slave_filter_ip");
	slave_filter_port = GetPrivateProfileInt((char *)global_conf.c_str(), (char *)"redis", (char *)"slave_filter_port");

	//20170708: add for new tracker
	master_price_ip = GetPrivateProfileString((char *)global_conf.c_str(), (char *)"redis", (char *)"master_price_ip");
	master_price_port = GetPrivateProfileInt((char *)global_conf.c_str(), (char *)"redis", (char *)"master_price_port");
	
	if ((slave_major_ip == NULL) || (slave_major_port == 0))
	{
		errcode = E_INVALID_PROFILE;
                goto exit;
	}
        if ((slave_filter_id_ip == NULL) || (slave_filter_id_port == 0))
	{
		errcode = E_INVALID_PROFILE;
                goto exit;
	}
	//if ((slave_ur_ip == NULL) || (slave_ur_port == 0))
	//{
	//	cout << "ok2.3" << endl;
	//	errcode = E_INVALID_PROFILE;
        //        goto exit;
	//}
	if ((pub_sub_ip == NULL) || (pub_sub_port == 0))
	{
		errcode = E_INVALID_PROFILE;
                goto exit;
	}
	if ((master_price_ip == NULL) || (master_price_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}
	if ((master_filter_ip == NULL) || (master_filter_port == 0))
	{
		errcode = E_INVALID_PROFILE;
                goto exit;
	}
	if ((slave_filter_ip == NULL) || (slave_filter_port == 0))
	{
		errcode = E_INVALID_PROFILE;
		goto exit;
	}

	ctx->slave_filter_id_ip = slave_filter_id_ip;
	ctx->slave_filter_id_port = slave_filter_id_port;
	ctx->master_price_ip = master_price_ip;     // 20170708: add for new tracker
	ctx->master_price_port = master_price_port; // 20170708: add for new tracker
//	ctx->slave_ur_ip = slave_ur_ip;
//	ctx->slave_ur_port = slave_ur_port;
	ctx->master_filter_ip = master_filter_ip;
	ctx->master_filter_port = master_filter_port;
	ctx->slave_filter_ip = slave_filter_ip;
	ctx->slave_filter_port = slave_filter_port;

	ctx->run_flag = run_flag;
	errcode = init_redis_operation(logid, ADX_MAX, slave_major_ip, slave_major_port, pub_sub_ip, pub_sub_port, run_flag, v_thread_id, &cache);
	if (errcode != E_SUCCESS)
	{
		goto exit;
	}
	ctx->cache = cache;

	initialize_conf_price(ctx->adx_path);
	make_success_message(ctx->messagedata);

	for(int i = 0; i < cpu_count; ++i)
	{
		GETCONTEXTINFO ctxinfo;

		redisContext *redis_id_slave = NULL;
//		redisContext *redis_ur_slave = NULL;
		redisContext *redis_price_master = NULL;  //20170708: add
		
		errcode = E_SUCCESS;//每次必须先初始化

		//2+
		redis_id_slave = redisConnect(slave_filter_id_ip, slave_filter_id_port);
		if ((redis_id_slave == NULL) || ((redis_id_slave != NULL) && (redis_id_slave->err != 0)))
		{
			writeLog(logid, LOGERROR, "Redis Connect failed, err:%d", E_REDIS_MASTER_FILTER_ID);
			errcode = E_REDIS_SLAVE_MAJOR;
			goto redis_proc;
		}


		//3+
	/*	redis_ur_slave = redisConnect(slave_ur_ip, slave_ur_port);
		if ((redis_ur_slave == NULL) || ((redis_ur_slave != NULL) && (redis_ur_slave->err != 0)))
		{
			writeLog(logid, LOGERROR, "Redis Connect failed, err:%d", E_REDIS_SLAVE_UR);
			errcode = E_REDIS_SLAVE_UR;
			goto redis_proc;
		}*/

		//4+  20170708: add for new tracker
		//errorcode.h add 0xE)
		redis_price_master = redisConnect(master_price_ip, master_price_port);
		if ((redis_price_master == NULL) || ((redis_price_master != NULL) && (redis_price_master->err != 0)))
		{
			writeLog(logid, LOGERROR, "Redis Connect failed, err:%d", E_REDIS_SLAVE_PRICE);
			errcode = E_REDIS_SLAVE_PRICE;
			goto redis_proc;
		}
		cout << "ok6.3" << endl;
redis_proc:
		if(errcode != E_SUCCESS)
		{
			if (redis_id_slave != NULL)
			{
				redisFree(redis_id_slave);
				redis_id_slave == NULL;
			}

			/*if (redis_ur_slave != NULL)
			{
				redisFree(redis_ur_slave);
				redis_ur_slave == NULL;
			}*/
			//20170708: add for new tracker
			if (redis_price_master != NULL)
			{
				redisFree(redis_price_master);
				redis_price_master == NULL;
			}

			goto exit;
		}

		ctxinfo.redis_id_slave = redis_id_slave;
//		ctxinfo.redis_ur_slave = redis_ur_slave;
		ctxinfo.redis_price_master = redis_price_master;  //20170708: add
		ctx->redis_fds.push_back(ctxinfo);
	} // end of for


	errcode = pthread_mutex_init(&ctx->mutex_price_info, 0);
	if (errcode != E_SUCCESS)
	{
		errcode = E_MUTEX_INIT;
		goto exit;
	}
	//errcode = pthread_create(&thread_t, NULL, thread_read_aprice, (void *)ctx);
	//if (errcode != E_SUCCESS)
	//	goto exit;
	//v_thread_id.push_back(thread_t);

exit:

	IFFREENULL(slave_filter_id_ip)
//	IFFREENULL(slave_ur_ip)
	IFFREENULL(slave_major_ip)
	IFFREENULL(pub_sub_ip)
	IFFREENULL(master_filter_ip)
	IFFREENULL(slave_filter_ip)
	IFFREENULL(master_price_ip)  // 20170708: add for new price
	if (errcode != E_SUCCESS)
	{
	   global_uninitialize((uint64)ctx);
	}
	else
	{
	   *pctx = (uint64_t)ctx;
	}

	return errcode;
}

int global_uninitialize(uint64 fctx)
{
	if (fctx != 0)
	{
		MONITORCONTEXT *pctx = (MONITORCONTEXT *)fctx;
		
		if (pctx->cache != 0)
		{
			uninit_redis_operation(pctx->cache);
			pctx->cache = 0;
		}

		for (int i = 0; i < pctx->adx_path.size(); ++i)
		{
			if (pctx->adx_path[i].dl_handle != NULL)
				dlclose(pctx->adx_path[i].dl_handle);
		}

		for (int i = 0; i < pctx->redis_fds.size(); ++i)
		{
			GETCONTEXTINFO &ctx = pctx->redis_fds[i];
			if (ctx.redis_id_slave)
			{
				redisFree(ctx.redis_id_slave);
				ctx.redis_id_slave = NULL;
			}
			/*if (ctx.redis_ur_slave)
			{
				redisFree(ctx.redis_ur_slave);
				ctx.redis_ur_slave = NULL;
			}*/

			//20170708: add for new tracker
			if (ctx.redis_price_master)
			{
				redisFree(ctx.redis_price_master);
				ctx.redis_price_master = NULL;
			}
		} // end of for
	    
		pthread_mutex_destroy(&pctx->mutex_price_info);
		delete pctx;
		
		return E_SUCCESS;
	} // end of if (fctx != 0)

	return E_INVALID_PARAM_CTX;
}
