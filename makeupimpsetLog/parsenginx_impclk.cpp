#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <dlfcn.h>
#include "../../common/errorcode.h"
#include "../../common/type.h"
#include "../../common/url_endecode.h"
#include "../../common/util.h"
#include "../../impclk/impclk.h"
using namespace std;

#define		PRICECONF			"dsp_price.conf"

fstream file_in;
fstream writeLogfd;
fstream sendLog_impclkfd;
fstream sendLog_settlefd;

vector<ADX_CONTENT> adx_path;

void init_ADX_CONTENT(ADX_CONTENT &adx_content)
{
	adx_content.name = 0;
	adx_content.dl_handle = NULL;
	return;
}

void* thread_conf_price(void *arg)
{
	ADX_CONTENT adx_content;
	/* if we want to delete the platfrom, vector must be refresh, the decode function also changes at the same time */
	for(int i = 1; i < ADX_MAX; i++)
	{
		char *path = NULL;
		char index[10] = "";
		string conf_price = string(GLOBAL_PATH) + string(PRICECONF);

		sprintf(index, "%d", i);

		path = GetPrivateProfileString((char *)conf_price.c_str(), (char *)"default", index);

		if (path != NULL)
		{
			init_ADX_CONTENT(adx_content);
			adx_content.name = i;

			adx_content.dl_handle = dlopen(path, RTLD_LAZY);

			adx_path.push_back(adx_content);
		}
		else
			break;
	} 
}

int Decode(int adx, char *encode_price, double &price)
{
	int errcode = E_SUCCESS;
	char *error = NULL;
	void *dl_handle = NULL;
	typedef int (*func)(char*, double*);

	if ((adx == 0) || (encode_price == NULL))
	{
		errcode = E_INVALID_PARAM;
		goto exit;
	}

	if(adx_path[adx-1].dl_handle != NULL)
	{
		func fc = (func)dlsym(adx_path[adx-1].dl_handle, "DecodeWinningPrice");

		error = dlerror();
		if(error != NULL)
		{
			errcode = E_DL_GET_FUNCTION_FAIL;
			goto exit;
		}

		errcode = fc(encode_price, &price);
		if (errcode != 0)
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
int parseRequest(char *requestparam, char *requestaddr, char *date)
{
	string str_flag = "";
	char *adxtype = NULL, *mtype = NULL, *mapid = NULL, *id = NULL, *deviceid = NULL, *deviceidtype = NULL, 
		*action = NULL, *destination_url = NULL, *send_data = NULL, *strprice = NULL, *send_data_s = NULL,
		*appsid = NULL, *advertiser = NULL;
	char slog[MAX_LENGTH] = "";

	double price = 0;
	int errcode = E_SUCCESS;
	uint8_t iadx = 0;
	char outSendImpclk[MAX_LENGTH];
	char outSendSettle[MAX_LENGTH];

	/* 1.Required Parameters */
	mtype = getParamvalue(requestparam, "mtype");
	mapid = getParamvalue(requestparam, "mapid");
	if (((mtype == NULL) || (strlen(mtype) > PXENE_MTYPE_LEN)) ||
		((mapid == NULL) || (strlen(mapid) > PXENE_MAPID_LEN)))
	{
		//cout<<__FILE__<<"  "<<__LINE__<<endl;
		errcode = E_INVALID_PARAM_REQUEST;
		goto exit;
	}

	/* 2. App monitor */
	appsid = getParamvalue(requestparam, "appsid");
	if ((appsid != NULL) && (strlen(appsid) < PXENE_APPSID_LEN))
	{
		advertiser = getParamvalue(requestparam, "adv");
		if ((advertiser != NULL) && (strlen(advertiser) <= PXENE_ADV_LEN))
		{
			//writeLog(g_logid_local, LOGDEBUG, "App monitor->appid:%s,advertiser:%", appsid, advertiser);
			goto redirection;
		}
		else
		{
			//cout<<__FILE__<<"  "<<__LINE__<<endl;
			errcode = E_INVALID_PARAM_REQUEST;
			//writeLog(g_logid_local, LOGERROR, "Request error, err:%x\n", E_INVALID_PARAM_REQUEST);
			goto exit;
		}
	}

	/* 3. Optional Parameters*/
	action = getParamvalue(requestparam, "action");
	if ((action != NULL) && (strlen(action) <= PXENE_ACTION_LEN))
	{
		id = getParamvalue(requestparam, "bid");

		if ((mtype[0] == 'v') && ((id != NULL) && (strlen(id) < PXENE_BID_LEN)))
		{
			//writeLog(g_logid_local, LOGDEBUG, "Action Monitor:%s", action);
			goto redirection;
		}
		else
		{
			//writeLog(g_logid_local, LOGERROR, "Request error, err:%x\n", E_INVALID_PARAM_REQUEST);
			//cout<<__FILE__<<"  "<<__LINE__<<endl;
			errcode = E_INVALID_PARAM_REQUEST;
			goto exit;
		}
	}

	/* 4. ADX 0~255 */
	adxtype = getParamvalue(requestparam, "adx");
	if ((adxtype == NULL) || (strlen(adxtype) > PXENE_ADX_LEN) || (iadx > 255))
	{
		//writeLog(g_logid_local, LOGERROR, "ADX was null or invalid, err:%x\n", E_INVALID_PARAM_REQUEST);
		//cout<<__FILE__<<"  "<<__LINE__<<endl;
		errcode = E_INVALID_PARAM_REQUEST;
		goto exit;
	}
	iadx = atoi(adxtype);

	/* 5. For third show and click */
	if (iadx == 0)
	{
		id = getParamvalue(requestparam, "bid");
		goto feedback;
	}

	id = getParamvalue(requestparam, "bid");
	deviceid = getParamvalue(requestparam, "deviceid");
	deviceidtype = getParamvalue(requestparam, "deviceidtype");
	if (((id == NULL) || (strlen(id) > PXENE_BID_LEN)) ||
		((deviceid == NULL) || (strlen(deviceid) > PXENE_DEVICEID_LEN)) ||
		((deviceidtype == NULL) || (strlen(deviceidtype) > PXENE_DEVICEIDTYPE_LEN)))
	{
		//writeLog(g_logid_local, LOGERROR, "ID/Deviceid/Deviceidtype was nulll or invalid, err:%x\n", E_INVALID_PARAM_REQUEST);
		//cout<<__FILE__<<"  "<<__LINE__<<endl;
		errcode = E_INVALID_PARAM_REQUEST;
		goto exit;
	}

	/* 6. Price */
	strprice = getParamvalue(requestparam, "price");
	if (strprice != NULL)
	{
		errcode = Decode(iadx, strprice, price);
	}

redirection:
	destination_url = getParamvalue(requestparam, "curl");
	if (destination_url != NULL)
	{
		int decode_len = urldecode(destination_url, strlen(destination_url));

		string destinationurl = "Location: " + string(destination_url) + "\n\n";

		if ((id != NULL) && (mapid != NULL))
		{
			replaceMacro(destinationurl, "#BID#", id);
			replaceMacro(destinationurl, "#MAPID#", mapid);
		}

		if (appsid != NULL)
		{
			sprintf(outSendImpclk, "appsid:%s,mapid:%s,mtype:%c,adv:%s,redirection:%s\n",appsid, mapid, mtype[0], advertiser, destination_url);
			writeLogfd.write(outSendImpclk, strlen(outSendImpclk));
			goto exit;
		}
	}

feedback:
	if (1)
	{
		if ((deviceid != NULL) && (deviceidtype != NULL))
		{
			sprintf(outSendImpclk, "%s|%s|%s|%s|%c|%d\n", id, deviceid, deviceidtype, mapid, mtype[0], iadx);
			if (strprice != NULL)
			{
				sprintf(outSendSettle, "%s|%s|%s|%s|%f|%d\n", id, deviceid, deviceidtype, mapid, price, iadx);
				sprintf(slog,  "%s.000,DBG,ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,mtype:%c,price:%f\n", date, requestaddr, iadx, id, mapid, deviceid, deviceidtype, mtype[0], price);
			}
			else
				sprintf(slog,  "%s.000,DBG,ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,mtype:%c\n", date, requestaddr, iadx, id, mapid, deviceid, deviceidtype, mtype[0]);
		}	
		else
		{
			if (id != NULL)
			{
				if (action != NULL)
				{
					sprintf(send_data, "|%s|%s|%s|%c|\n", id, mapid, action, mtype[0]);//the third monitor,exist bid
					sprintf(slog,  "%s.000,DBG,ip:%s,bid:%s,mapid:%s,mtype:%c,action:%s\n", date, requestaddr, mtype[0], mapid, id, action);
				}
				else
				{
					sprintf(send_data, "||%s|%s|%c|%d\n", id, mapid, mtype[0], iadx);//the third request,exist bid
					sprintf(slog,  "%s.000,DBG,ip:%s,adx:%d,bid:%s,mapid:%s,mtype:%c\n", date, requestaddr, iadx, id, mapid, mtype[0]);
				}			
			}
			else
			{
				sprintf(send_data, "|||%s|%c|%d\n", mapid, mtype[0], iadx);//the inmobi dsp request,do not exist bid
				sprintf(slog,  "%s.000,DBG,ip:%s,adx:%d,mapid:%s,mtype:%c\n", date, requestaddr, iadx, mapid, mtype[0]);
			}
		}

		if ((mtype[0] != 'a') && (mtype[0] != 'v'))
		{
			sendLog_impclkfd.write(outSendImpclk, strlen(outSendImpclk));

			if ((strprice != NULL))
			{
				sendLog_settlefd.write(outSendSettle, strlen(outSendSettle));
			}
		}
		writeLogfd.write(slog, strlen(slog));
	}

exit:

	if (adxtype != NULL)
		free(adxtype);

	if (mtype != NULL)
		free(mtype);

	if (mapid != NULL)
		free(mapid);

	if (strprice != NULL)
		free(strprice);

	if (id != NULL)
		free(id);

	if (deviceid != NULL)
		free(deviceid);

	if (deviceidtype != NULL)
		free(deviceidtype);

	if (action != NULL)
		free(action);

	if (destination_url != NULL)
		free(destination_url);

	if (send_data_s != NULL)
		free(send_data_s);

	if (send_data != NULL)
		free(send_data);

	if (advertiser != NULL)
		free(advertiser);

	if (appsid != NULL)
		free(appsid);

	return errcode;
}

void unit_dll_lib()
{
	for (int j = 0;j < adx_path.size(); j++)
	{
		if(adx_path[j].dl_handle != NULL)
		{
			cout <<"name " <<adx_path[j].name <<endl;
			dlclose(adx_path[j].dl_handle);
		}
	}
	adx_path.clear();
	return;
}

int main(int argc,char **argv)
{
	if(argc < 2)
	{
		cout<<"input file path:"<<endl;
		return 0;
	}
	string filename = argv[1];
	string sendLogName;
	string writeLogName;
	string truename = "";

	file_in.open(argv[1], ios_base::in);

	truename = filename + "_impclk.log";
	writeLogfd.open(truename.c_str(), ios_base::out);

	truename = filename + "_sendimpclk.log";
	sendLog_impclkfd.open(truename.c_str(), ios_base::out);
	truename = filename + "_sendsettle.log";
	sendLog_settlefd.open(truename.c_str(), ios_base::out);

	char requestparam[4096]; 
	char url[1024];
	char IP[20];
	char date[64];
	char time[64];

	thread_conf_price(0);

	if(file_in.is_open())
	{
		char *adx, *mtype, *mapid, *id, *deviceid, *deviceidtype, *price;
		int errorcode = E_SUCCESS;
		while(file_in.getline(requestparam, 4096))
		{ 
			sscanf(requestparam, "%s %s %s", IP, date, url);
			cout <<"IP :" <<IP <<endl;
			cout <<"date :" <<date << endl;
			cout <<"url :" <<url << endl;
			errorcode = parseRequest(url, IP, date);
			if(errorcode == E_SUCCESS)
			{
				cout <<"parse success" << endl;
			}
			else
				printf("errorcode :0x:%x\n", errorcode);
		} 
		file_in.close();
		writeLogfd.close();
		sendLog_impclkfd.close();
		sendLog_settlefd.close();
	}

	unit_dll_lib();
	return 0;
}
