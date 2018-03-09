#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>
#include "openrtb.pb.h"
#include "zplay_adx_rtb.pb.h"
#include "../../common/json_util.h"
#include "../../common/url_endecode.h"
#include "zplay_response.h"
#include <string.h>

extern uint64_t g_logid, g_logid_local, g_logid_response;

static bool TranferToCommonRequest(IN com::google::openrtb::BidRequest &zplay_request, OUT COM_REQUEST &crequest, OUT com::google::openrtb::BidResponse &zplay_response)
{
	int errorcode = E_SUCCESS;
	bool berr = true;

	if(zplay_request.id() != "")
	{
		return true;
		zplay_response.set_id(zplay_request.id());
		crequest.id = zplay_request.id();
	}	
	else
	{
		return false;
	}

	if(zplay_request.has_device())
	{

		writeLog(g_logid_local, LOGERROR, "zplayrequest.device.os:%s", zplay_request.device().os().c_str());
		if(!strcasecmp(zplay_request.device().os().c_str(), "Android"))
			crequest.device.os = OS_ANDROID;
		else if(!strcasecmp(zplay_request.device().os().c_str(), "iOS"))
			crequest.device.os = OS_IOS;
		else if(!strcasecmp(zplay_request.device().os().c_str(), "WP"))
			crequest.device.os = OS_WINDOWS;
		else crequest.device.os = OS_UNKNOWN;


		crequest.device.osv = zplay_request.device().osv();
		crequest.device.model = zplay_request.device().model();
		crequest.device.ip = zplay_request.device().ip();
		crequest.device.ua = zplay_request.device().ua();


		if(crequest.device.os == OS_ANDROID)
		{
			if(zplay_request.device().macsha1() != "")
			{
				crequest.device.dids.insert(make_pair(DID_MAC_SHA1,zplay_request.device().macsha1()));
				writeLog(g_logid_local, LOGERROR, "zplayrequest.device.macsha1:%s",zplay_request.device().macsha1().c_str());
			}

			if(zplay_request.device().didsha1() != "")
			{
				crequest.device.dids.insert(make_pair(DID_IMEI_SHA1,zplay_request.device().didsha1()));
				writeLog(g_logid_local, LOGERROR, "zplayrequest.device.didsha1:%s",zplay_request.device().didsha1().c_str());
			}

			if (zplay_request.device().dpidsha1() != "")
				crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1,zplay_request.device().dpidsha1()));

		}
		if(crequest.device.os == OS_IOS)
		{
			if (zplay_request.device().dpidsha1() != "")
				crequest.device.dpids.insert(make_pair(DPID_IDFA_SHA1,zplay_request.device().dpidsha1()));

		}

		switch (zplay_request.device().connectiontype())
		{
		case 0:
			crequest.device.connectiontype = CONNECTION_UNKNOWN;
			break;
		case 2:
			crequest.device.connectiontype = CONNECTION_WIFI;
			break;
		case 3:
			crequest.device.connectiontype = CONNECTION_CELLULAR_UNKNOWN;
			break;
		case 4:
			crequest.device.connectiontype = CONNECTION_CELLULAR_2G;
			break;
		case 5:
			crequest.device.connectiontype = CONNECTION_CELLULAR_3G;
			break;
		case 6:
			crequest.device.connectiontype = CONNECTION_CELLULAR_4G;
			break;
		default:
			crequest.device.connectiontype = CONNECTION_UNKNOWN;
			break;
			
		}

		switch (zplay_request.device().devicetype())
		{
		case 1:
		case 4:
			crequest.device.devicetype = DEVICE_PHONE;
			break;
		case 5:
			crequest.device.deviceidtype = DEVICE_TABLET;
			break;
		default:
			crequest.device.devicetype = DEVICE_UNKNOWN;
			break;
		}

		crequest.device.geo.lat = zplay_request.device().geo().lat();
		crequest.device.geo.lon = zplay_request.device().geo().lon();
		
		if(zplay_request.device().GetExtension(zadx::plmn) != "")
		{
			if(zplay_request.device().GetExtension(zadx::plmn) == "46000")
				crequest.device.carrier = CARRIER_CHINAMOBILE;
			else if(zplay_request.device().GetExtension(zadx::plmn) == "46001")
				crequest.device.carrier = CARRIER_CHINAUNICOM;
			else if(zplay_request.device().GetExtension(zadx::plmn) == "46002")
				crequest.device.carrier = CARRIER_CHINATELECOM;			
			else 
				crequest.device.carrier = CARRIER_UNKNOWN;
		}

		if(zplay_request.device().GetExtension(zadx::imei) != "")
			crequest.device.dids.insert(make_pair(DID_IMEI,zplay_request.device().GetExtension(zadx::imei)));

		if(zplay_request.device().GetExtension(zadx::mac) != "")
			crequest.device.dids.insert(make_pair(DID_MAC,zplay_request.device().GetExtension(zadx::mac)));

		if(zplay_request.device().GetExtension(zadx::android_id) != "")
			crequest.device.dpids.insert(make_pair(DPID_ANDROIDID,zplay_request.device().GetExtension(zadx::android_id)));
	}
	else 
	{
		errorcode = E_BAD_REQUEST;
		berr = false;
		goto exit;
	}

	if(zplay_request.imp_size() > 0)
	{
		for(int i = 0; i < zplay_request.imp_size(); ++i)
		{
			com::google::openrtb::BidResponse_SeatBid *seatbid = zplay_response.add_seatbid();
			
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);

			imp.id = zplay_request.imp(i).id();
			imp.bidfloor = zplay_request.imp(i).bidfloor() / 100; //转化为元/CPM
			imp.bidfloorcur = "CNY";
			imp.ext.instl = zplay_request.imp(i).instl();

			if(zplay_request.imp(i).has_video())
			{
				imp.type = IMP_VIDEO;
				imp.video.minduration = zplay_request.imp(i).video().minduration();
				imp.video.maxduration = zplay_request.imp(i).video().maxduration();
				imp.video.w = zplay_request.imp(i).video().w();
				imp.video.h = zplay_request.imp(i).video().h();
			}
			else if(zplay_request.imp(i).has_banner())
			{
				imp.type = IMP_BANNER;
				imp.banner.w = zplay_request.imp(i).banner().w();
				imp.banner.h = zplay_request.imp(i).banner().h();
			}
			else
			{
				errorcode = E_BAD_REQUEST;
				berr = false;
				goto exit;
			}

			imp.ext.splash = zplay_request.imp(i).GetExtension(zadx::is_splash_screen);
			crequest.imp.push_back(imp);
		}
	}
	else
	{
		errorcode = E_BAD_REQUEST;
		berr = false;
		goto exit;
	}

	writeLog(g_logid_local, LOGERROR, "TransferToCommonRequest  end...");

exit:
	if(!berr)
   		writeLog(g_logid_local, LOGERROR, "Bad Request, errcode:0x%x", errorcode);

	return berr;
}

bool zplayResponse(IN uint8_t index, IN RECVDATA *arg, OUT string &senddata)
{
	int errorcode = E_SUCCESS;
	string outputdata = "";
	RECVDATA *recvdata = arg;

	COM_REQUEST comrequest;
	com::google::openrtb::BidRequest zplay_request;

	COM_RESPONSE comresponse;
	com::google::openrtb::BidResponse zplay_response;

	bool parsesuccess = false;

	if (recvdata == NULL)
	{
		writeLog(g_logid_local, LOGERROR, string("recvdata is NULL"));
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		writeLog(g_logid_local, LOGERROR, string("recvdata->data is null or recvdata->length is 0"));
		goto exit;
	}

	init_com_message_request(&comrequest);

	parsesuccess = zplay_request.ParseFromArray(recvdata->data, recvdata->length);

	if(!parsesuccess)
	{
	//	writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		goto exit;
	}
	
	parsesuccess = TranferToCommonRequest(zplay_request, comrequest, zplay_response);

	if (parsesuccess)
		//senddata = "Content-Length: " + intToString(outputdata.size()) + "\r\n\r\n" + outputdata;
		senddata = "Status: 204 OK\r\n\r\n";

exit:

	if (recvdata->data != NULL)
	{
		free(recvdata->data);
		recvdata->data = NULL;
	}

	if (recvdata != NULL)
	{
		free(recvdata);
		recvdata = NULL;
	}
	
	return parsesuccess;
}































