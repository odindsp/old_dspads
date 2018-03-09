#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <hiredis/hiredis.h>
#include "impclk.h"
#include "../common/json_util.h"

using namespace std;

extern uint64_t g_logid_local;

extern "C"
{
#include <dlfcn.h>
	//  #include <ccl/ccl.h>
}

uint16_t time_count = 0;


int parseRequest(IN FCGX_Request request, IN DATATRANSFER *datatransfer, IN char *requestaddr, IN char *requesturi, IN char *requestparam)
{
	char *adxtype = NULL, *mtype = NULL, *mapid = NULL, *id = NULL, *deviceid = NULL, *deviceidtype = NULL;
	//char slog[MAX_LENGTH] = "";
	char send_data[MAX_LENGTH] = "";
	int  errcode = E_SUCCESS;
	uint8_t iadx = 0;

	if ((datatransfer == NULL) || (requestaddr == NULL) || (requesturi == NULL) || (requestparam == NULL))
	{
		errcode = E_INVALID_PARAM;
		writeLog(g_logid_local, LOGERROR, "request params invalid,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	/* 1.Required Parameters */
	mtype = getParamvalue(requestparam, "mtype");
	mapid = getParamvalue(requestparam, "mapid");
	if (((mtype == NULL) || (strlen(mtype) > PXENE_MTYPE_LEN)) ||
		((mapid == NULL) || (!check_string_type(mapid, PXENE_MAPID_LEN, true, false, INT_RADIX_16))))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "mtype/mapid was null or invalid,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	/* 4. ADX 0~255 */
	adxtype = getParamvalue(requestparam, "adx");
	if ((adxtype == NULL) || (strlen(adxtype) > PXENE_ADX_LEN) || (iadx > 255))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "adx was null or invalid,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	iadx = atoi(adxtype);
	id = getParamvalue(requestparam, "bid");
	deviceid = getParamvalue(requestparam, "deviceid");
	deviceidtype = getParamvalue(requestparam, "deviceidtype");
	if (((id == NULL) || (strlen(id) > PXENE_BID_LEN)) ||
		((deviceid == NULL) || (strlen(deviceid) > PXENE_DEVICEID_LEN)) ||
		((deviceidtype == NULL) || (strlen(deviceidtype) > PXENE_DEVICEIDTYPE_LEN)))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "id/deviceid/deviceidtype was nulll or invalid,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}

	sprintf(send_data, "%s|%s|%s|%s|%c|%d", id, deviceid, deviceidtype, mapid, mtype[0], iadx);
	//sprintf(slog, "ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,mtype:%c", requestaddr, iadx, id, mapid, deviceid, deviceidtype, mtype[0]);
	sendLog(datatransfer, string(send_data, strlen(send_data)));
exit:

	if (adxtype != NULL)
		free(adxtype);

	if (mtype != NULL)
		free(mtype);

	if (mapid != NULL)
		free(mapid);

	if (id != NULL)
		free(id);

	if (deviceid != NULL)
		free(deviceid);

	if (deviceidtype != NULL)
		free(deviceidtype);
	return errcode;
}
