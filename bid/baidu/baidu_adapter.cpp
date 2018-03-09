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
#include "baidu_adapter.h"
using namespace std;

extern map< uint32_t, vector<int> > outputadcat;         // out
extern map<int, vector<uint32_t> > inputadcat;           // in
extern map<int, vector<uint32_t> > appcattable;
extern uint64_t g_logid_local, g_logid, g_logid_response;
extern map<string, uint16_t> dev_make_table;
extern uint64_t geodb;
extern int *numcount;

// in,key-int
void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int, vector<uint32_t> > &adcat = *((map<int, vector<uint32_t> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}
// out,key-hex
void transfer_adv_hex(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<int> > &adcat = *((map<uint32_t, vector<int> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%x", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%d", &value);
	vector<int> &cat = adcat[key];
	cat.push_back(value);
}

uint64_t iab_to_uint64(string cat)
{
	cout << "cat = " << cat << endl;
	int icat = 0;
	sscanf(cat.c_str(), "%d", &icat);
	return icat;
}

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

void initialize_no_device(OUT COM_DEVICEOBJECT &device, IN const string &ip, IN const string &useragent)
{
	//    device.didtype = DID_UNKNOWN;
	//    device.dpidtype = DPID_UNKNOWN;
	device.carrier = CARRIER_UNKNOWN;
	device.ip = ip;
	device.ua = useragent;
	device.os = OS_UNKNOWN;
	device.geo.lat = 0;
	device.geo.lon = 0;
	device.connectiontype = CONNECTION_UNKNOWN;
	device.devicetype = DEVICE_UNKNOWN;

	return;
}

void initialize_have_device(OUT COM_DEVICEOBJECT &device, IN const BidRequest_Mobile &mobile, 
	IN const string &ip, IN const string &useragent, IN string bid)
{
	device.ip = ip;
	device.ua = useragent;

	if (device.ip != "")
	{
		device.location = getRegionCode(geodb, device.ip);
	}

	if (mobile.device_type() == BidRequest_Mobile::HIGHEND_PHONE)
		device.devicetype = DEVICE_PHONE;
	else if (mobile.device_type() == BidRequest_Mobile::TABLET)
		device.devicetype = DEVICE_TABLET;

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
	device.model = mobile.model();

	if (mobile.has_os_version())
	{
		char buf[64];
		sprintf(buf, "%d.%d.%d", 
			mobile.os_version().os_version_major(), 
			mobile.os_version().os_version_minor(), 
			mobile.os_version().os_version_micro());
		device.osv = buf;
	}

	int32_t carrier = mobile.carrier_id();
	if (carrier == 46000 || carrier == 46002 || carrier == 46007 || carrier == 46008 || carrier == 46020)
		device.carrier = CARRIER_CHINAMOBILE;
	else if (carrier == 46001 || carrier == 46006 || carrier == 46009)
		device.carrier = CARRIER_CHINAUNICOM;
	else if (carrier == 46003 || carrier == 46005 || carrier == 46011)
		device.carrier = CARRIER_CHINATELECOM;
	else
		writeLog(g_logid_local, LOGDEBUG, "id :%s and device carrier :%d", bid.c_str(), carrier);

	if (mobile.wireless_network_type() == BidRequest_Mobile::WIFI)
	{
		device.connectiontype = CONNECTION_WIFI;
	}
	else if (mobile.wireless_network_type() == BidRequest_Mobile::MOBILE_2G)
	{
		device.connectiontype = CONNECTION_CELLULAR_2G;
	}
	else if (mobile.wireless_network_type() == BidRequest_Mobile::MOBILE_3G)
	{
		device.connectiontype = CONNECTION_CELLULAR_3G;
	}
	else if (mobile.wireless_network_type() == BidRequest_Mobile::MOBILE_4G)
	{
		device.connectiontype = CONNECTION_CELLULAR_4G;
	}

	if (mobile.platform() == BidRequest_Mobile::IOS)
	{
		device.os = OS_IOS;
	}
	else if (mobile.platform() == BidRequest_Mobile::ANDROID)
	{
		device.os = OS_ANDROID;
	}

	for (uint32_t i = 0; i < mobile.id_size(); ++i)
	{
		const BidRequest_Mobile_MobileID &did = mobile.id(i);
		string lowid = did.id();
		writeLog(g_logid_local, LOGINFO, "did=%s", lowid.c_str());
		transform(lowid.begin(), lowid.end(), lowid.begin(), ::tolower);
		if (did.type() == BidRequest_Mobile_MobileID::IMEI)
			device.dids.insert(make_pair(DID_IMEI_MD5, lowid));
		else if (did.type() == BidRequest_Mobile_MobileID::MAC)
			device.dids.insert(make_pair(DID_MAC_MD5, lowid));
	}

	//     device.os = OS_IOS;                        // test
	//     device.dpidtype = DPID_IDFA;             // test
	//     device.dpid = "47b9caf8-2b0e-4398-8399-0a712a706bc4";    // test   
	if (device.os == OS_UNKNOWN)
	{
		writeLog(g_logid_local, LOGERROR, "os is unknown");
		goto returnback;
	}
	for (uint32_t i = 0; i < mobile.for_advertising_id_size(); ++i)     // test 注释掉
	{

		const BidRequest_Mobile_ForAdvertisingID &devid = mobile.for_advertising_id(i);
		writeLog(g_logid_local, LOGINFO, "devid=%s", devid.id().c_str());
		if (device.os == OS_IOS && devid.type() == BidRequest_Mobile_ForAdvertisingID::IDFA)
		{
			//     device.dpidtype = DPID_IDFA;
			//     device.dpid = devid.id();
			device.dpids.insert(make_pair(DPID_IDFA, devid.id()));
			break;
		}
		else if (device.os == OS_ANDROID && devid.type() == BidRequest_Mobile_ForAdvertisingID::ANDROID_ID)
		{
			//    device.dpidtype = DPID_ANDROIDID;
			//   device.dpid = devid.id();
			device.dpids.insert(make_pair(DPID_ANDROIDID, devid.id()));
			break;
		}
	}

returnback:
	return;
}

void set_bcat(IN BidRequest &bidrequest, OUT set<uint32_t> &bcat)
{
	for (uint32_t i = 0; i < bidrequest.excluded_product_category_size(); ++i)
	{
		int category = bidrequest.excluded_product_category(i);
		vector<uint32_t> &adcat = inputadcat[category];
		for (uint32_t j = 0; j < adcat.size(); ++j)
			bcat.insert(adcat[j]);
	}
	return;
}

// convert baidu request to common request
int parse_common_request(IN BidRequest &bidrequest, OUT COM_REQUEST &commrequest, OUT BidResponse &bidresponse)
{
	int errorcode = E_SUCCESS;
	string ip, useragent;

	init_com_message_request(&commrequest);
	if (bidrequest.has_id())
	{
		bidresponse.set_id(bidrequest.id());
		commrequest.id = bidrequest.id();
	}
	else
	{
		errorcode = E_REQUEST_NO_BID;
		goto badrequest;
	}

	if (bidrequest.is_ping())
	{
		errorcode = E_REQUEST_IS_PING;
		goto badrequest;
	}

	ip = bidrequest.ip();
	useragent = bidrequest.user_agent();

	// baidu now support one adzone
	writeLog(g_logid_local, LOGINFO, "bid=%s,adslot_size=%d", bidrequest.id().c_str(), bidrequest.adslot_size());
	for (uint32_t i = 0; i < bidrequest.adslot_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;
		init_com_impression_object(&impressionobject);
		impressionobject.bidfloorcur = "CNY";

		const BidRequest_AdSlot &adslot = bidrequest.adslot(i);
		int32_t id = adslot.sequence_id();
		char tempid[1024] = "";
		sprintf(tempid, "%d", id);
		impressionobject.id = tempid;

		impressionobject.bidfloor = (double)(adslot.minimum_cpm() / 100.0);

		int adpos = adslot.slot_visibility();
		if (adpos == 1)
		{
			impressionobject.adpos = ADPOS_FIRST;
		}
		else if (adpos == 2)
		{
			impressionobject.adpos = ADPOS_SECOND;
		}
		else if (adpos == 0)
		{
			impressionobject.adpos = ADPOS_OTHER;
		}
		// get size
		int w = adslot.width();
		int h = adslot.height();

		int32_t viewtypetemp = -1;
		if (adslot.has_adslot_type())
			viewtypetemp = adslot.adslot_type();
		writeLog(g_logid_local, LOGINFO, "bid=%s,viewtype=%d", bidrequest.id().c_str(), viewtypetemp);
		if (is_banner(viewtypetemp)) // banner
		{
			impressionobject.type = IMP_BANNER;
			impressionobject.banner.w = w;
			impressionobject.banner.h = h;

			//set ext
			if (viewtypetemp == 11)
				impressionobject.ext.instl = 1;
			else if (viewtypetemp == 12)
				impressionobject.ext.splash = 1;

			set<int> creativetype;
			//            creativetype.insert(2); // test
			// 根据允许的广告创意退出拒绝的广告创意类型
			for (uint32_t j = 0; j < adslot.creative_type_size(); ++j)
			{
				va_cout("creati_type = %d", adslot.creative_type(j));
				writeLog(g_logid_local, LOGINFO, "bid=%s,creati_type=%d", bidrequest.id().c_str(), adslot.creative_type(j));
				switch (adslot.creative_type(j))
				{
				case 0:
					creativetype.insert(1);
					break;
				case 1:
					creativetype.insert(2);
					break;
				case 2:
					creativetype.insert(7);
					break;
				case 7:
					creativetype.insert(6);
					break;
				default:
					//           creativetype.insert(0);       //
					break;
				}
			}
			if (creativetype.count(0))
			{
				writeLog(g_logid_local, LOGERROR, "bid=%s creativetype count 0", bidrequest.id().c_str());
				continue;
			}
			for (uint32_t j = ADTYPE_TEXT_LINK; j <= ADTYPE_FEEDS; ++j)
			{
				if (!creativetype.count(j))
					impressionobject.banner.btype.insert(j);
			}

		}
		else
		{
			va_cout("other type");
			errorcode = E_REQUEST_IMP_TYPE;
			if (is_video(viewtypetemp))  // video
			{
				impressionobject.type = IMP_VIDEO;
				impressionobject.video.w = w;
				impressionobject.video.h = h;

				// 设定视频支持的格式
				impressionobject.video.mimes.insert(VIDEOFORMAT_X_FLV);
				if (adslot.has_video_info())
				{
					impressionobject.video.minduration = (uint32_t)adslot.video_info().min_video_duration();
					impressionobject.video.maxduration = (uint32_t)adslot.video_info().max_video_duration();
				}

			}
			else
				continue;
		}

		if (adslot.secure())
			commrequest.is_secure = 1;
		else
			commrequest.is_secure = 0;
		commrequest.imp.push_back(impressionobject);
	}

	if (bidrequest.has_mobile())  // app bundle
	{
		const  BidRequest_Mobile &mobile = bidrequest.mobile();
		if (mobile.has_mobile_app())
		{
			commrequest.app.id = mobile.mobile_app().app_id();
			commrequest.app.bundle = mobile.mobile_app().app_bundle_id();
			commrequest.app.storeurl = bidrequest.url();

			// 设置app的cat
			int32_t app_cat = mobile.mobile_app().app_category();
			va_cout("appcat = %d", app_cat);
			vector<uint32_t> &appcat = appcattable[app_cat];
			va_cout("app size= %d", appcat.size());
			for (uint32_t i = 0; i < appcat.size(); ++i)
				commrequest.app.cat.insert(appcat[i]);
		}
		initialize_have_device(commrequest.device, mobile, ip, useragent, bidrequest.id());
		// 设置device的经纬度
		if (bidrequest.has_user_geo_info())
		{
			if (bidrequest.user_geo_info().user_coordinate_size() > 0)
			{
				commrequest.device.geo.lat = bidrequest.user_geo_info().user_coordinate(0).latitude();
				commrequest.device.geo.lon = bidrequest.user_geo_info().user_coordinate(0).longitude();
			}
		}

		if (commrequest.device.dids.size() == 0 && commrequest.device.dpids.size() == 0)
		{
			errorcode = E_REQUEST_DEVICE_ID;
			writeLog(g_logid_local, LOGERROR, "bid=%s dids and dpids size is zero!", bidrequest.id().c_str());
			goto release;
		}
	}
	else    // set device null
	{
		errorcode = E_REQUEST_DEVICE;
		// initialize_no_device(commrequest.device,ip,useragent);
		writeLog(g_logid_local, LOGERROR, "bid=%s have no mobile", bidrequest.id().c_str());
		goto release;
	}
	if (0 == commrequest.imp.size())  // 没有符合的曝光对象
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "bid=%s imp size is zero!", bidrequest.id().c_str());
		goto release;
	}

	if (commrequest.device.location == 0 || commrequest.device.location > 1156999999 || commrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			bidrequest.id().c_str(), commrequest.device.ip.c_str(), commrequest.device.location);
		errorcode = E_REQUEST_DEVICE_IP;
	}

	MY_DEBUG;

	release :
	set_bcat(bidrequest, commrequest.bcat);  // 需要改正0806
	commrequest.cur.insert(string("CNY"));

badrequest:
	return errorcode;
}

// set response ad category
void set_response_category_new(IN set<uint32_t> &cat, OUT BidResponse_Ad *bidresponseads)
{
	if (NULL == bidresponseads)
		return;

	set<uint32_t>::iterator pcat = cat.begin();
	bidresponseads->set_category(9901);
	return;
}

void print_bid_object(COM_BIDOBJECT &bidobject)
{
	cout << "impid = " << bidobject.impid << endl;
	cout << "price = " << bidobject.price << endl;
	cout << "adid = " << bidobject.adid << endl;
	set<uint32_t>::iterator p = bidobject.cat.begin();
	for (p; p != bidobject.cat.end(); ++p)
	{
		cout << "cat = " << (int)*p << endl;
	}
	cout << "type = " << (int)bidobject.type << endl;
	cout << "ftype = " << (int)bidobject.ftype << endl;
	cout << "ctype = " << (int)bidobject.ctype << endl;
	cout << "bundle = " << bidobject.bundle << endl;
	cout << "apkname = " << bidobject.apkname << endl;
	cout << "adomain = " << bidobject.adomain << endl;
	cout << "w = " << bidobject.w << endl;
	cout << "h = " << bidobject.h << endl;
	cout << "curl = " << bidobject.curl << endl;
	cout << "redirect = " << (int)bidobject.redirect << endl;
	cout << "monitorcode = " << bidobject.monitorcode << endl;
	for (uint32_t i = 0; i < bidobject.cmonitorurl.size(); ++i)
		cout << "cmonitorurl = " << bidobject.cmonitorurl[i] << endl;
	for (uint32_t i = 0; i < bidobject.imonitorurl.size(); ++i)
		cout << "imonitorurl = " << bidobject.imonitorurl[i] << endl;
	cout << "sourceurl = " << bidobject.sourceurl << endl;
	cout << "cid = " << bidobject.cid << endl;
	cout << "crid = " << bidobject.crid << endl;

	return;
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
void parse_common_response(IN COM_REQUEST &commrequest,IN COM_RESPONSE &commresponse,IN ADXTEMPLATE &tanxmessage,OUT BidResponse &bidresponse)
{
#ifdef DEBUG
    cout<<"id = "<<commresponse.id<<endl;
    cout<<"bidid = "<<commresponse.bidid<<endl;
#endif
// replace result
    int adcount = 0;
    for (uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
    {
        COM_SEATBIDOBJECT &seatbidobject = commresponse.seatbid[i];
        for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
        {
            COM_BIDOBJECT &bidobject = seatbidobject.bid[j];
#ifdef DEBUG
            print_bid_object(bidobject);
#endif
            BidResponse_Ad *bidresponseads = bidresponse.add_ad();
            if (NULL == bidresponseads)
                continue;

            int impid = atoi(bidobject.impid.c_str());
            bidresponseads->set_sequence_id(impid);
            writeLog(g_logid_local,LOGINFO, "bid=%s,impid=%d",commresponse.id.c_str(),impid);

            // 设置广告id和广告主id
             string creatid = bidobject.creative_ext.id;
             writeLog(g_logid_local,LOGINFO, "bid=%s,creatid=%s",commresponse.id.c_str(),creatid.c_str());
             bidresponseads->set_creative_id(atoll(creatid.c_str()));
             va_cout("creatid = %lld\n",atoll(creatid.c_str()));
            //bidresponseads->set_advertiser_id();

            double price = bidobject.price * 100;
            writeLog(g_logid_local,LOGINFO, "bid=%s,price=%lf",commresponse.id.c_str(),price);
            bidresponseads->set_max_cpm((int32_t)(price));

 //           bidresponseads->set_is_cookie_matching(false);

            // 设置extdata,待定，是否需要修改
			char ext[MID_LENGTH] = "";
            sprintf(ext, "%s", get_trace_url_par(commrequest, commresponse).c_str());

            va_cout("extdata = %s",ext);
            bidresponseads->set_extdata(ext);

        }
    }

    return;
}

void print_comm_request(IN COM_REQUEST &commrequest)
{
	cout << "id = " << commrequest.id << endl;
	cout << "impression:" << endl;
	for (uint32_t i = 0; i < commrequest.imp.size(); ++i)
	{
		cout << "id = " << commrequest.imp[i].id << endl;
		cout << "imptype = " << (int)commrequest.imp[i].type << endl;
		if (commrequest.imp[i].type == IMP_BANNER) // bannner
		{
			cout << "w = " << commrequest.imp[i].banner.w << endl;
			cout << "h = " << commrequest.imp[i].banner.h << endl;
			set<uint8_t>::iterator p = commrequest.imp[i].banner.btype.begin();
			while (p != commrequest.imp[i].banner.btype.end())
			{
				cout << "btype = " << (int)*p << endl;
				++p;
			}
		}
		else if (commrequest.imp[i].type == IMP_VIDEO)
		{
			cout << "video mindur = " << commrequest.imp[i].video.minduration << endl;
			cout << "video maxdur = " << commrequest.imp[i].video.maxduration << endl;
			cout << "video w = " << commrequest.imp[i].video.w << endl;
			cout << "video h = " << commrequest.imp[i].video.h << endl;
		}

		cout << "bidfloor = " << commrequest.imp[i].bidfloor << endl;
	}

	cout << "app:" << endl;
	cout << "id = " << commrequest.app.id << endl;
	cout << "name = " << commrequest.app.name << endl;
	cout << "bundle = " << commrequest.app.bundle << endl;
	cout << "storeurl = " << commrequest.app.storeurl << endl;
	for (set<uint32_t>::iterator pcat = commrequest.app.cat.begin(); pcat != commrequest.app.cat.end(); ++pcat)
	{
		printf("app cat = 0x%x\n", *pcat);
	}

	cout << "device:" << endl;
	map<uint8_t, string>::iterator dids = commrequest.device.dids.begin();
	for (; dids != commrequest.device.dids.end(); ++dids)
	{
		cout << "did = " << dids->second << endl;
		cout << "didtype = " << (int)dids->first << endl;
	}

	dids = commrequest.device.dpids.begin();
	for (; dids != commrequest.device.dpids.end(); ++dids)
	{
		cout << "dpid = " << dids->second << endl;
		cout << "dpidtype = " << (int)dids->first << endl;
	}
	cout << "ua = " << commrequest.device.ua << endl;
	cout << "ip = " << commrequest.device.ip << endl;
	cout << "lat = " << commrequest.device.geo.lat << endl;
	cout << "lon = " << commrequest.device.geo.lon << endl;
	cout << "carrier = " << (int)commrequest.device.carrier << endl;
	cout << "make = " << commrequest.device.make << endl;
	cout << "model = " << commrequest.device.model << endl;
	cout << "os = " << (int)commrequest.device.os << endl;
	cout << "osv = " << commrequest.device.osv << endl;
	cout << "connectiontype = " << (int)commrequest.device.connectiontype << endl;
	cout << "devicetype = " << (int)commrequest.device.devicetype << endl;

	set<uint32_t>::iterator p = commrequest.bcat.begin();
	while (p != commrequest.bcat.end())
	{
		printf("bcat = 0x%x\n", *p);
		++p;
	}

	return;
}

void baidu_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid)
{
	char *senddata = NULL;
	uint32_t sendlength = 0;
	BidRequest bidrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST commrequest;
	COM_RESPONSE commresponse;
	BidResponse bidresponse;
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

	err = parse_common_request(bidrequest, commrequest, bidresponse);
	if (err == E_SUCCESS)
	{
		flow_bid.set_req(commrequest);
		ferrcode = bid_filter_response(ctx, index, commrequest,
			NULL, NULL, filter_bid_price, filter_bid_ext,
			&commresponse, &adxtemplate);

		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		err = ferrcode.errcode;
		flow_bid.set_bid_flow(ferrcode);
		g_ser_log.send_bid_log(index, commrequest, commresponse, err);
	}
	else
	{
		writeLog(g_logid_local, LOGERROR, "the request format is error!");
		va_cout("it is a wrong request format");
	}
	
	if (err == E_SUCCESS)
	{
		parse_common_response(commrequest, commresponse, adxtemplate, bidresponse);
		va_cout("parse common response to baidu response success!");
	}
	else if (err == E_REDIS_CONNECT_INVALID)
	{
		syslog(LOG_INFO, "redis connect invalid");
		kill(getpid(), SIGTERM);
	}

	sendlength = bidresponse.ByteSize();
	senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
	if (senddata == NULL)
	{
		va_cout("allocate memory failure!");
		writeLog(g_logid_local, LOGERROR, "allocate memory failure!");
		goto release;
	}
	bidresponse.SerializeToArray(senddata, sendlength);

	data = "Content-Type: application/octet-stream\r\nContent-Length: " + 
		intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

	if (err == E_SUCCESS)
	{
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
	}

	bidresponse.clear_ad();
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
