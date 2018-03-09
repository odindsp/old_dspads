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
	for(int i = 1; i < ADX_MAX + 1; i++)
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
	char *adxtype = NULL, *encode_price = NULL, *mapid = NULL, *id = NULL, *deviceid = NULL, *deviceidtype = NULL;
	char slog[MAX_LENGTH] = "";

	double price = 0;
	int errcode = E_SUCCESS;
	uint8_t iadx = 0;
	char outSendSettle[MAX_LENGTH];

	/* 1.Required Parameters */
	id = getParamvalue(requestparam, "bid");
	mapid = getParamvalue(requestparam, "mapid");
	adxtype = getParamvalue(requestparam, "adx");
	encode_price = getParamvalue(requestparam, "price");
	deviceid = getParamvalue(requestparam, "deviceid");
	deviceidtype = getParamvalue(requestparam, "deviceidtype");
	
	if ((encode_price == NULL) ||
		((id == NULL) || (strlen(id) > PXENE_BID_LEN)) ||
		((mapid == NULL) || (strlen(mapid) > PXENE_MAPID_LEN)) ||
		((adxtype == NULL) || (strlen(adxtype) > PXENE_ADX_LEN)) || 
		((deviceid == NULL) || (strlen(deviceid) > PXENE_DEVICEID_LEN)) ||
		((deviceidtype == NULL) || (strlen(deviceidtype) > PXENE_DEVICEIDTYPE_LEN)))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		goto exit;
	}

	iadx = atoi(adxtype);

	errcode = Decode(iadx, encode_price, price);
	if ((errcode != E_SUCCESS) || (price < 0))
	{
		cout << "decode error" <<endl;
		goto exit;
	}

	if ((deviceid != NULL) && (deviceidtype != NULL ))
		sprintf(outSendSettle, "%s|%s|%s|%s|%f|%d\n", id, deviceid, deviceidtype, mapid, price, iadx);
	else
		sprintf(outSendSettle, "%s|||%s|%f|%d\n", id, mapid, price, iadx);

	sendLog_settlefd.write(outSendSettle, strlen(outSendSettle));

	sprintf(slog,  "%s.000,DBG,ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,price:%f\n", date, requestaddr, iadx, id, mapid, deviceid, deviceidtype, price);

	writeLogfd.write(slog, strlen(slog));

exit:
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

	truename = filename + "_settle.log";
	writeLogfd.open(truename.c_str(), ios_base::out);

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
		sendLog_settlefd.close();
	}
cout <<__LINE__ <<endl;
	unit_dll_lib();

	return 0;
}
