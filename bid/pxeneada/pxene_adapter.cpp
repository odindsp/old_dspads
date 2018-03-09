#include <syslog.h>
#include <signal.h>
#include <map>
#include <vector>
#include <string>
#include <errno.h>
#include <algorithm>
#include <set>
#include <google/protobuf/text_format.h>
#include "../../common/bid_filter.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "pxene_adapter.h"
using namespace std;

extern uint64_t g_logid_local, g_logid, g_logid_response;
extern map<string, uint16_t> dev_make_table;
extern uint64_t geodb;
extern int *numcount;

bool is_banner(IN int32_t viewtype)
{
	if (viewtype == 0 || viewtype == 1 || viewtype == 11 || viewtype == 12 || viewtype == 26)
		return true;
	else
		return false;
}

bool is_video(IN int32_t viewtype)
{
	if (viewtype == 21 || viewtype == 22 || viewtype == 23)
		return true;
	else
		return false;
}


void initialize_have_device(OUT COM_DEVICEOBJECT &device, IN const PBDevice &mobile, IN string bid)
{
	device.ip = mobile.ip();
	device.ua = mobile.ua();

	if (device.ip != "")
	{
		device.location = getRegionCode(geodb, device.ip);
	}


	device.devicetype = mobile.devicetype();
	device.model = mobile.model();

	string s_make = mobile.brand();
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;

		device.makestr = s_make;
		for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				device.make = it->second;
				break;
			}
		}
	}
	device.os = mobile.os();
	device.osv = mobile.osv();
	device.connectiontype = mobile.connectiontype();


	//     device.os = OS_IOS;                        // test
	//     device.dpidtype = DPID_IDFA;             // test
	//     device.dpid = "47b9caf8-2b0e-4398-8399-0a712a706bc4";    // test   
	if (device.os == OS_UNKNOWN)
	{
		writeLog(g_logid_local, LOGERROR, "os is unknown");
		goto returnback;
	}

	if (mobile.mac() != "")
	{
		device.dids.insert(make_pair(DID_MAC, mobile.mac()));
	}

	if (device.os == OS_IOS)
	{
		if (mobile.dpid() != "")
			device.dpids.insert(make_pair(DPID_IDFA, mobile.dpid()));
	}
	else if (device.os == OS_ANDROID)
	{
		if (mobile.did() != "")
		{
			device.dids.insert(make_pair(DID_IMEI, mobile.did()));
		}
		if (mobile.dpid() != "")
			device.dpids.insert(make_pair(DPID_ANDROIDID, mobile.dpid()));
	}


returnback:
	return;
}

// convert baidu request to common request
int parse_common_request(IN PBBidRequest &bidrequest, OUT COM_REQUEST &commrequest)
{
	int errorcode = E_SUCCESS;

	init_com_message_request(&commrequest);
	if (bidrequest.has_id())
	{
		commrequest.id = bidrequest.id();
	}
	else
	{
		errorcode |= E_BAD_REQUEST;
		goto badrequest;
	}

	for (uint32_t i = 0; i < bidrequest.imp_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;
		init_com_impression_object(&impressionobject);

		const PBImpression &adslot = bidrequest.imp(i);

		impressionobject.id = adslot.id();
		impressionobject.bidfloor = (double)(adslot.bidfloor() / 100.0);
		impressionobject.bidfloorcur = adslot.bidfloorcur();
		impressionobject.adpos = adslot.adpos();
		impressionobject.ext.instl = adslot.instl();
		if (adslot.has_ext())
			impressionobject.ext.splash = adslot.ext().splash();

		if (adslot.has_banner()) // banner
		{
			impressionobject.type = IMP_BANNER;
			impressionobject.banner.w = adslot.banner().w();
			impressionobject.banner.h = adslot.banner().h();

			for (uint32_t j = 0; j < adslot.banner().battr_size(); ++j)
			{
				impressionobject.banner.battr.insert(adslot.banner().battr(j));
			}
		}
		else
		{
			va_cout("other type");
			errorcode |= E_INVALID_PARAM;
		}

		if (!adslot.is_clickable())
		{
			va_cout("not click");
			errorcode |= E_IMP_NOT_CLICK;
		}

		if (adslot.secure())
			commrequest.is_secure = 1;
		else
			commrequest.is_secure = 0;
		commrequest.imp.push_back(impressionobject);
	}

	if (bidrequest.has_user())
	{
		commrequest.user.id = bidrequest.user().id();
		commrequest.user.gender = bidrequest.user().gender();
		commrequest.user.yob = bidrequest.user().yob();
		commrequest.user.keywords = bidrequest.user().keywords();
		commrequest.user.searchkey = bidrequest.user().searchkey();
		if (bidrequest.user().has_geo())
		{
			commrequest.user.geo.lat = bidrequest.user().geo().lat();
			commrequest.user.geo.lon = bidrequest.user().geo().lon();
		}
	}

	if (bidrequest.has_app())
	{
		commrequest.app.id = bidrequest.app().id();
		commrequest.app.name = bidrequest.app().name();
		commrequest.app.bundle = bidrequest.app().bundle();
		commrequest.app.storeurl = bidrequest.app().storeurl();

		// 设置app的cat
		for (uint32_t i = 0; i < bidrequest.app().cat_size(); ++i)
			commrequest.app.cat.insert(bidrequest.app().cat(i));
	}

	if (bidrequest.has_device())  // app bundle
	{
		const  PBDevice &mobile = bidrequest.device();
		initialize_have_device(commrequest.device, mobile, bidrequest.id());
		// 设置device的经纬度
		/*  if (bidrequest.has_user_geo_info())
		  {
		  if (bidrequest.user_geo_info().user_coordinate_size() > 0)
		  {
		  commrequest.device.geo.lat = bidrequest.user_geo_info().user_coordinate(0).latitude();
		  commrequest.device.geo.lon = bidrequest.user_geo_info().user_coordinate(0).longitude();
		  }
		  }*/

		if (commrequest.device.dids.size() == 0 && commrequest.device.dpids.size() == 0)
		{
			errorcode |= E_DEVICEID_NOT_EXIST;
			writeLog(g_logid_local, LOGERROR, "bid=%s dids and dpids size is zero!", bidrequest.id().c_str());
			goto release;
		}
	}
	else    // set device null
	{
		errorcode |= E_DEVICE_NOT_EXIST;
		// initialize_no_device(commrequest.device,ip,useragent);
		writeLog(g_logid_local, LOGERROR, "bid=%s have no mobile", bidrequest.id().c_str());
		goto release;
	}
	if (0 == commrequest.imp.size())  // 没有符合的曝光对象
	{
		errorcode |= E_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "bid=%s imp size is zero!", bidrequest.id().c_str());
		goto release;
	}

	if (commrequest.device.location == 0 || commrequest.device.location > 1156999999 || commrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			bidrequest.id().c_str(), commrequest.device.ip.c_str(), commrequest.device.location);
		errorcode |= E_REQUEST_DEVICE_IP;
	}

	MY_DEBUG
	release :

	for (uint32_t i = 0; i < bidrequest.bcat_size(); ++i)
	{
		commrequest.bcat.insert(bidrequest.bcat(i));
	}

	for (uint32_t i = 0; i < bidrequest.badv_size(); ++i)
	{
		commrequest.badv.insert(bidrequest.badv(i));
	}
	commrequest.cur.insert(string("CNY"));
badrequest:
	return errorcode;
}

static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if ((price - crequest.imp[0].bidfloor - 0.01) >= 0.000001)
		return true;

	return false;
}
static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &ext)
{
	bool filter = true;

	if (ext.id == "")
		filter = false;

	return filter;
}

// convert common response to abidu response
void parse_common_response(IN COM_REQUEST &commrequest, IN COM_RESPONSE &commresponse, IN ADXTEMPLATE &tanxmessage, OUT PBBidResponse &bidresponse)
{
#ifdef DEBUG
	cout << "id = " << commresponse.id << endl;
	cout << "bidid = " << commresponse.bidid << endl;
#endif
	string trace_url_par = get_trace_url_par(commrequest, commresponse);
	bidresponse.set_id(commresponse.id);
	bidresponse.set_bidid(commresponse.id);
	bidresponse.set_cur("CNY");
	// replace result
	int adcount = 0;
	for (uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = commresponse.seatbid[i];
		PBSeatBid *bidresponseatbid = bidresponse.add_seatbid();

		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT &bidobject = seatbidobject.bid[j];
			PBBid *bidresponseads = bidresponseatbid->add_bid();
			if (NULL == bidresponseads)
				continue;

			bidresponseads->set_impid(bidobject.impid);


			// 设置广告id和广告主id
			string creatid = bidobject.creative_ext.id;
			writeLog(g_logid_local, LOGINFO, "bid=%s,creatid=%s", commresponse.id.c_str(), creatid.c_str());
			bidresponseads->set_adid(creatid);

			double price = bidobject.price * 100;
			writeLog(g_logid_local, LOGINFO, "bid=%s,price=%lf", commresponse.id.c_str(), price);
			bidresponseads->set_bid_price((price));
			va_cout("extdata = %s", trace_url_par.c_str());
			bidresponseads->set_extdata(trace_url_par.c_str());
		}
	}

	return;
}

void baidu_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid)
{
	char *senddata = NULL;
	uint32_t sendlength = 0;
	PBBidRequest bidrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;

	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST commrequest;
	COM_RESPONSE commresponse;
	PBBidResponse bidresponse;
	uint8_t viewtype = 0;
	ADXTEMPLATE adxtemplate;
	struct timespec ts3, ts4;
	long lasttime2 = 0;
	string str_ferrcode = "";

	getTime(&ts3);

	if (recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
	{
		err = E_BID_PROGRAM;
		writeLog(g_logid_local, LOGERROR, "recv data is error!");
		goto release;
	}

	parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		err = E_BAD_REQUEST;
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		va_cout("parse data from array failed!");
		goto release;
	}

	err = parse_common_request(bidrequest, commrequest);

	if (err == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGERROR, "the request format is error!");
		va_cout("it is a wrong request format");
		goto release;
	}
	else if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "id = %s the request is not suit for bidding!", bidrequest.id().c_str());
		va_cout("the request is not suit for bidding!");
		goto returnresponse;
	}

	flow_bid.set_req(commrequest);
	ferrcode = bid_filter_response(ctx, index, commrequest, NULL, NULL, filter_bid_price, filter_bid_ext, &commresponse, &adxtemplate);
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	flow_bid.set_bid_flow(ferrcode);
	err = ferrcode.errcode;

	g_ser_log.send_bid_log(index, commrequest, commresponse, err);

	if (err == E_SUCCESS)
	{
		parse_common_response(commrequest, commresponse, adxtemplate, bidresponse);
		va_cout("parse common response to baidu response success!");
	}

returnresponse:
	if (err == E_REDIS_CONNECT_INVALID)
	{
		syslog(LOG_INFO, "redis connect invalid");
		kill(getpid(), SIGTERM);
	}

	if (err == E_SUCCESS)
	{
		sendlength = bidresponse.ByteSize();
		senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
		if (senddata == NULL)
		{
			va_cout("allocate memory failure!");
			writeLog(g_logid_local, LOGERROR, "allocate memory failure!");
			goto release;
		}
		bidresponse.SerializeToArray(senddata, sendlength);

		data = "Content-Type: application/x-protobuf\r\nContent-Length: " 
			+ intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

		numcount[index]++;
		if (numcount[index] % 1000 == 0)
		{
			string str_response;
			google::protobuf::TextFormat::PrintToString(bidresponse, &str_response);
			writeLog(g_logid_local, LOGDEBUG, "response=%s", str_response.c_str());
			numcount[index] = 0;
		}
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", 
			commresponse.id.c_str(), commresponse.seatbid[0].bid[0].mapid.c_str(), 
			commrequest.device.deviceid.c_str(), commrequest.device.deviceidtype, 
			commresponse.seatbid[0].bid[0].price);
		bidresponse.clear_seatbid();
	}
	else
	{
		data = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts4);
	lasttime2 = (ts4.tv_sec - ts3.tv_sec) * 1000000 + (ts4.tv_nsec - ts3.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, bidrequest.id().c_str(), lasttime2, err, str_ferrcode.c_str());
	va_cout("%s,spent time:%ld,err:0x%x,desc:%s", bidrequest.id().c_str(), lasttime2, err, str_ferrcode.c_str());

release:
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

	if (senddata != NULL)
	{
		free(senddata);
		senddata = NULL;
	}

	MY_DEBUG;
	flow_bid.set_err(err);
	return;
}
