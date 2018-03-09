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

	id = getParamvalue(requestparam,"id");	//���ҳ
	if(id == NULL)
	{
		errcode = E_PV_INVALID_ID;
		writeLog(g_logid_local, LOGERROR, "id is null,err:0x%x", errcode);
		goto exit;
	}
#ifdef DEBUG
	cout<<"id:"<<id<<endl;
#endif
	ty = getParamvalue(requestparam, "ty");	//��������ͣ�1������(FIRST_JUMP)��2������(OUT_JUMP)��3������(TWICE_JUMP)
	if(ty == NULL)
	{
		errcode = E_PV_INVALID_TY;
		goto exit;
	}
#ifdef DEBUG
	cout<<"ty:"<<ty<<endl;
#endif
	bid = getParamvalue(requestparam,"bid");	//bid
	mid = getParamvalue(requestparam,"mid");	//����id
	datasrc = getParamvalue(requestparam,"datasrc"); //������Դƽ̨	
	//Ŀǰ�� dsp �� pap ƽ̨��NULL �Ķ��� dsp ƽ̨
	if (datasrc == NULL)
	{
		datasrc = (char *)calloc(4, sizeof(char));
		strncpy(datasrc, "dsp", 4);
	}
	
	sys = getParamvalue(requestparam,"sys");	//����ϵͳ
	ds = getParamvalue(requestparam,"ds");		//�ƶ��豸�ֱ���
	if(sys == NULL || ds == NULL)
	{
		errcode = E_PV_INVALID_SYSDS;
		goto exit;
	}
#ifdef DEBUG
	cout<<"sys:"<<sys<<endl;
	cout<<"ds:"<<ds<<endl;
#endif	
	location = getRegionCode(geodb, addr);	//����λ��
	if(location == 0)
	{
		writeLog(g_logid_local, LOGERROR, "id:%s,location is zero", id);
	}
	
	//���´���Ĳ�����Ϊǰ̨���û��ֵ,sendLogΪ""�Ĳ���(�����������͹�������)
	tp = getParamvalue(requestparam,"tp");	//�ƶ�����(false)
	op = getParamvalue(requestparam,"op");	//��Ӫ��(false)
	nw = getParamvalue(requestparam,"nw");	//��������(false)
	mb = getParamvalue(requestparam,"mb");  //�豸�ͺ�
	
	ity = atoi(ty);
	pu = getParamvalue(requestparam, "pu");	//��������(����true)
	cl = getParamvalue(requestparam, "cl");  //�ܷ�ҳ��

	if(ity == FIRST_JUMP && cl == NULL)	
	{
	   errcode = E_PV_INVALID_CL;
	   goto exit;
	}

	//��ͬ������ͬ����
	if(ity == FIRST_JUMP)	//��������
	{
		pv = getParamvalue(requestparam, "pv");	//չ����
		uv = getParamvalue(requestparam, "uv");	//�û���
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
		//����������������
		hf = getParamvalue(requestparam, "hf");	//������URL
		if(ity == TWICE_JUMP && hf == NULL) //������hf������Ҫ��ȡ
		{
			errcode = E_PV_INVALID_HF;
			goto exit;
		}
		at = getParamvalue(requestparam, "at");	//ƽ������ʱ��	
		if(at == NULL)
		{
			errcode = E_PV_INVALID_AT;
			goto exit;
		}
#ifdef DEBUG
	cout<<"at:"<<at<<endl;
#endif	

	    time = getParamvalue(requestparam, "time");	//ƽ������ʱ��	
		if(time == NULL)
		{
			errcode = E_PV_INVALID_AT;
			goto exit;
		}
	}
	
	if(ity == FIRST_JUMP)	//��������
	{
		sprintf(send_data, "%s|%s|%s|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s|%s", id, pv, uv, location, requestaddr, NULLSTR(pu), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, cl, NULLSTR(mb), datasrc);	//û�еĲ���˵��������ʱ���ƶ�����tp����Ӫ��op����������nw
		sprintf(slog, "id:%s,pv:%s,uv:%s,location:%d,ip:%s,pu:%s,tp:%s,sys:%s,ds:%s,op:%s,nw:%s,bid:%s,mid:%s,ity:%d,cl:%s,mb:%s,datasrc:%s", id, pv, uv, location, requestaddr, NULLSTR(pu), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, cl, NULLSTR(mb), datasrc);
	}
	else if(ity == OUT_JUMP)	//��������
	{
		sprintf(send_data, "%s|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s|%s|%s", id, location, requestaddr, at, NULLSTR(pu), NULLSTR(hf), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);	//û�еĲ���˵��������ʱ����������pu������hf���ƶ�����tp����Ӫ��op����������nw
		sprintf(slog, "id:%s,location:%d,ip:%s,at:%s,pu:%s,hf:%s,tp:%s,sys:%s,ds:%s,op:%s,nw:%s,bid:%s,mid:%s,ity:%d,cl:%s,mb:%s,time:%s,datasrc:%s", id, location, requestaddr, at, NULLSTR(pu), NULLSTR(hf), NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);
	}
	else				//��������
	{
		sprintf(send_data, "%s|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s|%s|%s", id, location, requestaddr, at, NULLSTR(pu), hf, NULLSTR(tp), sys, ds, NULLSTR(op), NULLSTR(nw), NULLSTR(bid), NULLSTR(mid), ity, NULLSTR(cl), NULLSTR(mb), time, datasrc);	//û�еĲ���˵��������ʱ����������pu���ƶ�����tp����Ӫ��op����������nw
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
