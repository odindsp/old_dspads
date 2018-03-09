#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <hiredis/hiredis.h>
#include "pv.h"

extern uint64_t g_logid_local, g_logid;
extern uint64_t geodb;

int parseRequest(IN FCGX_Request request, IN DATATRANSFER *datatransfer, IN char *requestaddr, IN char *requestparam)
{
	char *id = NULL, *bid = NULL, *mid = NULL, *ty = NULL, 
		 *pv = NULL, *uv = NULL, *pu = NULL, *sys = NULL, 
		 *ds = NULL, *at = NULL, *hf = NULL, *tp = NULL, 
		 *op = NULL, *nw = NULL, *cl = NULL, *mb = NULL, *time = NULL;
	char *datasrc = NULL;
	struct timespec ts1, ts2;
	char slog[MAX_LENGTH] = "";
	long lasttime = 0;
	int location = 0;
	int length = 0;
	int errcode = E_SUCCESS;
	uint8_t ity = 0;
	char send_data[MAX_LENGTH] = "";
	string addr;

	getTime(&ts1);
	if ((datatransfer == NULL) || (requestaddr == NULL) || (requestparam == NULL))
	{
		errcode = E_INVALID_PARAM;
		writeLog(g_logid_local, LOGERROR, "URL request params invalid,err:0x%x", errcode);
		goto exit;
	}
	
	addr = requestaddr;

	id = getParamvalue(requestparam,"id");	//落地页
	if(id == NULL)
	{
		errcode = E_PV_INVALID_ID;
		writeLog(g_logid_local, LOGERROR, "id is null,err:0x%x", errcode);
		goto exit;
	}
#ifdef DEBUG
	cout<<"id:"<<id<<endl;
#endif
	ty = getParamvalue(requestparam, "ty");	//请求的类型：1、访问(FIRST_JUMP)；2、跳出(OUT_JUMP)；3、二跳(TWICE_JUMP)
	if(ty == NULL)
	{
		errcode = E_PV_INVALID_TY;
		goto exit;
	}
#ifdef DEBUG
	cout<<"ty:"<<ty<<endl;
#endif
	bid = getParamvalue(requestparam,"bid");	//bid
	mid = getParamvalue(requestparam,"mid");	//创意id
	datasrc = getParamvalue(requestparam,"datasrc"); //数据来源平台	
	//目前仅 dsp 和 pap 平台，NULL 的都是 dsp 平台
	if (datasrc == NULL)
	{
		datasrc = (char *)calloc(4, sizeof(char));
		strncpy(datasrc, "dsp", 4);
	}
	
	sys = getParamvalue(requestparam,"sys");	//操作系统
	ds = getParamvalue(requestparam,"ds");		//移动设备分别率
	if(sys == NULL || ds == NULL)
	{
		errcode = E_PV_INVALID_SYSDS;
		goto exit;
	}
#ifdef DEBUG
	cout<<"sys:"<<sys<<endl;
	cout<<"ds:"<<ds<<endl;
#endif	
	location = getRegionCode(geodb, addr);	//地理位置
	if(location == 0)
	{
		writeLog(g_logid_local, LOGERROR, "id:%s,location is zero", id);
	}
	
	//以下处理的参数是为前台入参没有值,sendLog为""的参数(三个请求类型公共参数)
	tp = getParamvalue(requestparam,"tp");	//移动类型(false)
	op = getParamvalue(requestparam,"op");	//运营商(false)
	nw = getParamvalue(requestparam,"nw");	//联网类型(false)
	mb = getParamvalue(requestparam,"mb");  //设备型号
	
	ity = atoi(ty);
	pu = getParamvalue(requestparam, "pu");	//上游连接(访问true)
	cl = getParamvalue(requestparam, "cl");  //受访页面

	if(ity == FIRST_JUMP && cl == NULL)	
	{
	   errcode = E_PV_INVALID_CL;
	   goto exit;
	}

	//不同的请求不同处理
	if(ity == FIRST_JUMP)	//访问请求
	{
		pv = getParamvalue(requestparam, "pv");	//展现数
		uv = getParamvalue(requestparam, "uv");	//用户数
		if(pv == NULL ||  uv == NULL)
		{
			errcode = E_PV_INVALID_PVUV;
			goto exit;
		}
#ifdef DEBUG
	cout<<"pv:"<<pv<<endl;
	cout<<"uv:"<<uv<<endl;
#endif	
	}
	else
	{	
		//跳出请求或二跳请求
		hf = getParamvalue(requestparam, "hf");	//二跳的URL
		if(ity == TWICE_JUMP && hf == NULL) //二跳有hf参数需要获取
		{
			errcode = E_PV_INVALID_HF;
			goto exit;
		}
		at = getParamvalue(requestparam, "at");	//平均访问时长	
		if(at == NULL)
		{
			errcode = E_PV_INVALID_AT;
			goto exit;
		}
#ifdef DEBUG
	cout<<"at:"<<at<<endl;
#endif	

	    time = getParamvalue(requestparam, "time");	//平均访问时长	
		if(time == NULL)
		{
			errcode = E_PV_INVALID_AT;
			goto exit;
		}
	}
	
	if(ity == FIRST_JUMP)	//访问请求
	{
		sprintf(send_data, "%s|%s|%s|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s|%s", id, pv, uv, location, requestaddr, NULLSTR(pu), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, cl, NULLSTR(mb), datasrc);	//没有的参数说明：访问时，移动类型tp、运营商op、联网类型nw
		sprintf(slog, "id:%s,pv:%s,uv:%s,location:%d,ip:%s,pu:%s,tp:%s,sys:%s,ds:%s,op:%s,nw:%s,bid:%s,mid:%s,ity:%d,cl:%s,mb:%s,datasrc:%s", id, pv, uv, location, requestaddr, NULLSTR(pu), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, cl, NULLSTR(mb), datasrc);
	}
	else if(ity == OUT_JUMP)	//跳出请求
	{
		sprintf(send_data, "%s|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s|%s|%s", id, location, requestaddr, at, NULLSTR(pu), NULLSTR(hf), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);	//没有的参数说明：跳出时，上游连接pu、二跳hf、移动类型tp、运营商op、联网类型nw
		sprintf(slog, "id:%s,location:%d,ip:%s,at:%s,pu:%s,hf:%s,tp:%s,sys:%s,ds:%s,op:%s,nw:%s,bid:%s,mid:%s,ity:%d,cl:%s,mb:%s,time:%s,datasrc:%s", id, location, requestaddr, at, NULLSTR(pu), NULLSTR(hf), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);
	}
	else				//二跳请求
	{
		sprintf(send_data, "%s|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s|%s|%s", id, location, requestaddr, at, NULLSTR(pu), hf, NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);	//没有的参数说明：二跳时，上游连接pu、移动类型tp、运营商op、联网类型nw
		sprintf(slog, "id:%s,location:%d,ip:%s,at:%s,pu:%s,hf:%s,tp:%s,sys:%s,ds:%s,op:%s,nw:%s,bid:%s,mid:%s,ity:%d,cl:%s,mb:%s,time:%s,datasrc:%s", id, location, requestaddr, at, NULLSTR(pu), hf, NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);
	}
	length = strlen(send_data);
	if(strlen(send_data) != 0)
	{
		sendLog(datatransfer, string(send_data, length));
#ifdef DEBUG
		cout<<"send_data: "<<string(send_data, length)<<endl;
#endif
	}
	if(strlen(slog))
	{
		writeLog(g_logid, LOGDEBUG, string(slog));
	}
exit:
	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
#ifdef DEBUG
	cout<<"lasttime:"<<lasttime<<endl;
#endif
	writeLog(g_logid_local, LOGDEBUG, "id:%s,spent lasttime:%ld,err:0x%x", id, lasttime, errcode);

	FREE(id);
	FREE(bid);
	FREE(datasrc);
	FREE(mid);
	FREE(ty);
	FREE(pv);
	FREE(uv);
	FREE(pu);
	FREE(sys);
	FREE(ds);
	FREE(at);
	FREE(hf);
	FREE(tp);
	FREE(op);
	FREE(nw);
	FREE(cl);
	FREE(mb);
	FREE(time);

#ifdef DEBUG
	cout << "odin_pv exit,errcode: " <<hex<<errcode << endl;
#endif
	return errcode;
}
