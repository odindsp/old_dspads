#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include "settle.h"
#include "../common/json_util.h"

extern "C"
{
	#include <dlfcn.h>
}

using namespace std;

#define		MAX_LEN		128

extern vector<string> cmds;
extern pthread_mutex_t g_mutex_id;
extern uint64_t g_logid, g_logid_local;

/* Filter the request which disappear within 3 second */
//bool filter_deviceid_ip(GETCONTEXTINFO *ctx, char *ip, char *deviceid, const char *type)
//{
//	redisReply *reply = NULL;
//	bool bip = false, bdeviceid = false, err = false;
//	char command_ip[MIN_LENGTH] = "", command_deviceid[MIN_LENGTH] = "";
//
//	if ((ctx == NULL) || (deviceid == NULL) || (type == NULL))
//	{
//		goto exit;
//	}
//
//	sprintf(command_ip, "EXISTS ip_%s_%s", type, ip);
//	sprintf(command_deviceid, "EXISTS deviceid_%s_%s", type, deviceid);
//
//	if (REDIS_OK != redisAppendCommand(ctx->redis_filter_slave, command_ip)
//	 || REDIS_OK != redisAppendCommand(ctx->redis_filter_slave, command_deviceid))
//	{
//			writeLog(g_logid_local, LOGERROR, "Failed to execute command EXISTS with pipline, err:%d\n", E_REDIS_MASTER_FILTER_ID);
//			goto exit;
//	}
//
//	if (REDIS_OK != redisGetReply(ctx->redis_filter_slave, (void**)&reply))
//	{
//			goto exit;
//	}
//	else
//	{
//		if(reply->integer != 0)
//			bip = true;
//
//		freeReplyObject(reply);
//	}
//
//	if (REDIS_OK != redisGetReply(ctx->redis_filter_slave, (void**)&reply))
//	{
//		goto exit;
//	}
//	else
//	{
//		if(reply->integer != 0)
//			bdeviceid = true;
//
//		freeReplyObject(reply);
//	}
//
//		if (bip || bdeviceid)
//		{
//			goto exit;
//		}
//		else
//		{
//			char set_ip[MAX_LEN] = "", set_deviceid[MAX_LEN] = "";
//
//			sprintf(set_ip, "SETEX ip_%s_%s 3 %s", type, ip, ip);
//			sprintf(set_deviceid, "SETEX deviceid_%s_%s 3 %s", type, deviceid, deviceid);
//
//			if (REDIS_OK != redisAppendCommand(ctx->redis_filter_master, set_ip)
//				|| REDIS_OK != redisAppendCommand(ctx->redis_filter_master, set_deviceid))
//			{
//				writeLog(g_logid_local, LOGERROR, "Failed to execute command SETEX with pipline, err:%d\n", E_REDIS_MASTER_FILTER_ID);
//				goto exit;
//			}
//
//			if (REDIS_OK != redisGetReply(ctx->redis_filter_master, (void**)&reply))
//			{
//				writeLog(g_logid_local, LOGERROR, "Failed to execute command setip with pipline, err:%d\n", E_REDIS_MASTER_FILTER_ID);
//				goto exit;
//			}
//			freeReplyObject(reply);
//
//			if (REDIS_OK != redisGetReply(ctx->redis_filter_master, (void**)&reply))
//			{
//				writeLog(g_logid_local, LOGERROR, "Failed to execute command setdeviceid with pipline, err:%d\n", E_REDIS_MASTER_FILTER_ID);
//				goto exit;
//		}
//			freeReplyObject(reply);
//	}
//
//		err = true;
//
//exit:
//		writeLog(g_logid_local, LOGERROR, "Failed to execute filter deviceid and ip!");
//		return err;
//}


/* settle:parse the accept request and send info to remote server */
int parseRequest(uint64_t gctx, uint8_t index, DATATRANSFER *datatransfer, char *requestaddr, char *requestparam)
{
	char *adxtype = NULL, *id = NULL, 
		*mapid = NULL, *deviceid = NULL, 
		*encode_price = NULL,
		*deviceidtype = NULL, *send_data = NULL;
	
	char *datasrc = NULL;

	char *appid, *nw, *os, *tp, *reqip, *gp, *mb, *op, *md;
	 appid = nw = os = tp = reqip = gp = mb = op = md = NULL;

	char slog[MAX_LENGTH] = "";
	char deappid[MAX_LENGTH] = "";
	char demodel[MAX_LENGTH] = "";
	string get_id_value = "" ;
	struct timespec ts1, ts2;

	 int i_nw, i_os, i_tp, i_op, i_mb, i_gp;
	 i_nw = i_os = i_tp = i_op = 0;
	 i_mb = 255;
	 i_gp = 1156000000;

	sem_t sem_id;
	uint64_t redis_info_addr = 0;
	bool bFilter = false, idflag = false;
	double price = 0;
	long lasttime = 0, request_len = 0, deviceidtype_len = 0;
	int iadx = 0, flag = 0, errcode = E_SUCCESS;
	ADX_PRICE_INFO temp_price_info;
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)gctx;
	GETCONTEXTINFO *ctxinfo = NULL;


	getTime(&ts1);

	if ((gctx == 0) || (datatransfer == NULL) || (requestaddr == NULL) || (requestparam == NULL))
	{
		errcode = E_INVALID_PARAM;
		goto release;
	}

	request_len = strlen(requestparam);

	id = getParamvalue(requestparam, "bid");
	mapid = getParamvalue(requestparam, "mapid");
	adxtype = getParamvalue(requestparam, "adx");
	encode_price = getParamvalue(requestparam, "price");
	deviceid = getParamvalue(requestparam, "deviceid");
	deviceidtype = getParamvalue(requestparam, "deviceidtype");
	datasrc = getParamvalue(requestparam, "datasrc"); //数据来源平台
	//目前仅 dsp 和 pap 平台，NULL 的都是 dsp 平台
	if (datasrc == NULL)
	{
		datasrc = (char *)calloc(4, sizeof(char));
		strncpy(datasrc, "dsp", 4);
	}
	
	appid = getParamvalue(requestparam, "appid");
	nw = getParamvalue(requestparam, "nw");
	os = getParamvalue(requestparam, "os");
	tp = getParamvalue(requestparam, "tp");
	reqip = getParamvalue(requestparam, "reqip");
	gp = getParamvalue(requestparam, "gp");
	mb = getParamvalue(requestparam, "mb");
	op = getParamvalue(requestparam, "op");
	md = getParamvalue(requestparam, "md");

	if(nw != NULL)
	{
		i_nw = atoi(nw);
	}
	if(os != NULL)
	{
		i_os = atoi(os);
	}
	if(tp != NULL)
	{
		i_tp = atoi(tp);
	}
	if(gp != NULL)
	{
		i_gp = atoi(gp);
	}
	if(mb != NULL)
	{
		i_mb = atoi(mb);
	}
	if(op != NULL)
	{
		i_op = atoi(op);
	}

	if ((encode_price == NULL) ||
		((id == NULL) || (strlen(id) > PXENE_BID_LEN)) ||
		((mapid == NULL) || (!check_string_type(mapid, PXENE_MAPID_LEN, true, false, INT_RADIX_16))) ||
		((adxtype == NULL) || (strlen(adxtype) > PXENE_ADX_LEN)) || 
		((deviceid == NULL) || (strlen(deviceid) > PXENE_DEVICEID_LEN)) ||
		((deviceidtype == NULL) || (strlen(deviceidtype) > PXENE_DEVICEIDTYPE_LEN)))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "Request invalid id:%s,mapid:%s,adx:%s,deviceid:%s,deviceidtype:%s,encode_price:%s, datasrc:%s",
			id, mapid, adxtype, deviceid, deviceidtype, encode_price, datasrc);
		goto release;
	}

	iadx = atoi(adxtype);

	/* Filter IP/DeviceID/ID */
	/*	bFilter = filter_deviceid_ip(ctx, requestaddr, deviceid, "s");
	if (!bFilter)//Add errorcode
	{
		writeLog(g_logid_local, LOGERROR, "The same ip/deviceid within 3 second, err:%d\n", E_REPEATED_REQUEST);
		goto release;
	}*/

	/* Decode price function */
	if (iadx == 0 || iadx > pctx->adx_path.size())   // 还未加载该adx的动态库
	{
		errcode = E_INVALID_PARAM;
		goto release;
	}
	
	errcode = Decode(pctx->adx_path[iadx - 1], encode_price, price);
	if ((errcode != E_SUCCESS) || (price < 0))
	{
		writeLog(g_logid_local, LOGERROR, "Decode error,bid:%s,errcode:0x%x,encode_price:%s", id, errcode, encode_price);
		goto release;
	}

	/* Get bid flag from redis */ 
	ctxinfo = &(pctx->redis_fds[index]);
	if (ctxinfo->redis_id_slave == NULL) //reconnect
	{
	   ctxinfo->redis_id_slave = redisConnect(pctx->slave_filter_id_ip.c_str(), pctx->slave_filter_id_port);
	   if (ctxinfo->redis_id_slave != NULL)
		{
			if (ctxinfo->redis_id_slave->err)
			{
				writeLog(g_logid_local, LOGDEBUG, "get id flag reconnect err:%s",ctxinfo->redis_id_slave->errstr);
				redisFree(ctxinfo->redis_id_slave);
				ctxinfo->redis_id_slave = NULL;
			}
		}
		else
		{
			writeLog(g_logid_local, LOGDEBUG, "get id flag reconnect NULL");
		}
	}

	if (ctxinfo->redis_id_slave != NULL)
	{
		errcode = getRedisValue(ctxinfo->redis_id_slave, g_logid_local, "dsp_id_flag_" + string(id), get_id_value);
		record_cost(g_logid_local, true, id, "dsp_id_flag", ts1);
		
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			redisFree(ctxinfo->redis_id_slave);
			ctxinfo->redis_id_slave = NULL;
		}

	    if (get_id_value != "")
		{
			flag = atoi(get_id_value.c_str());
			if (!(flag & DSP_FLAG_PRICE))
			{
			//	char flagid[10] = "";
				string command = "";

			//	sprintf(flagid, "%d", flag | DSP_FLAG_PRICE);
			//	command = "SETEX dsp_id_flag_" + string(id) + " " + intToString(REQUESTIDLIFE) + " " + string(flagid);

				command = "INCRBY dsp_id_flag_" + string(id) + " " + intToString(DSP_FLAG_PRICE);
				idflag = true;

				pthread_mutex_lock(&g_mutex_id);
				cmds.push_back(command);
				pthread_mutex_unlock(&g_mutex_id);

				//get_semaphore(pctx->cache, &redis_info_addr, &sem_id);
				//if(semaphore_release(sem_id)) //semaphore add 1
				//{
				//	REDIS_INFO *redis_cashe = (REDIS_INFO *)redis_info_addr;

				//	map<string, IMPCLKCREATIVEOBJECT>::iterator m_creative_it = redis_cashe->creativeinfo.find(string(mapid));
				//	if (m_creative_it != redis_cashe->creativeinfo.end())
				//	{
				//		IMPCLKCREATIVEOBJECT &creativeinfo = m_creative_it->second;

				//		temp_price_info.adx = iadx;
				//		temp_price_info.groupid = creativeinfo.groupid;
				//		temp_price_info.mapid = mapid;
				//		temp_price_info.price = price;

				//		pthread_mutex_lock(&pctx->mutex_price_info);
				//		pctx->v_price_info.push_back(temp_price_info);
				//		pthread_mutex_unlock(&pctx->mutex_price_info);
				//	}

				//	if (!semaphore_wait(sem_id))
				//		writeLog(g_logid_local, LOGERROR, "semaphore_wait failed");

				//	record_cost(g_logid_local, true, (id != NULL ? id:"NULL"), "semaphore_wait after", ts1);

				//}
			}
			else
			{
				errcode = E_REPEATED_REQUEST;
				writeLog(g_logid_local, LOGERROR, "duplication request:%s,index=%d,adx=%d", id, index, iadx);
				goto release;
			}
		}
		else
		{
			writeLog(g_logid_local, LOGERROR, "%s does not exist,index=%d,adx=%d", id, index, iadx);
			goto release;
		}
	}
feedback:
	if (idflag)
	{
		int length = 0;

		send_data = (char *)malloc(sizeof(char) * request_len);
		if(send_data != NULL)
		{
			sprintf(send_data, "%s|%s|%s|%s|%f|%d|%s|%s|%d|%d|%d|%d|%d|%d|%s|%s", id, deviceid, deviceidtype, mapid,
					price, iadx, requestaddr, NULLSTR(appid), i_nw, i_os, i_tp, i_gp, i_mb, i_op, NULLSTR(md), datasrc);
			length = strlen(send_data);
			sendLog(datatransfer, string(send_data, length));
		}
		
		if (appid != NULL)
		{
		  strncpy(deappid, appid, MAX_LENGTH - 1);
		  deappid[MAX_LENGTH - 1] = '\0';
		  urldecode(deappid, strlen(deappid));
		}

		if (md != NULL)
		{
		  strncpy(demodel, md, MAX_LENGTH - 1);
		  demodel[MAX_LENGTH - 1] = '\0';
		  urldecode(demodel, strlen(demodel));
		}
	
		sprintf(slog, "ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,appid:%s,network:%d,os:%d,devicetype:%d,reqip:%s,reqloc:%d,brand:%d,carrier:%d,model:%s,price:%f,datasrc:%s",
							requestaddr, iadx, id, mapid, deviceid, deviceidtype, deappid, i_nw, i_os, i_tp, NULLSTR(reqip), i_gp, i_mb, i_op, demodel, price, datasrc);
		writeLog(g_logid, LOGDEBUG, string(slog));
	}

release:
	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "%s,spent time:%ld,err:0x%x" ,id, lasttime, errcode);
		
	if(adxtype != NULL)
		free(adxtype);
		
	if (id != NULL)
		free(id);
		
	if (mapid != NULL)
		free(mapid);
	
	if (datasrc != NULL)
        free(datasrc);
		
	if(deviceidtype != NULL)
		free(deviceidtype);
		
	if (encode_price != NULL)
		free(encode_price);
	
	if (deviceid != NULL)
		free(deviceid);

	if (send_data != NULL)
		free(send_data);

	if (appid != NULL)
		free(appid);

	if (nw != NULL)
		free(nw);

	if (os != NULL)
		free(os);

	if (tp != NULL)
		free(tp);

	if (reqip != NULL)
		free(reqip);

	if (gp != NULL)
		free(gp);

	if (mb != NULL)
		free(mb);

	if (op != NULL)
		free(op);

	if (md != NULL)
		free(md);

	return errcode;
}
