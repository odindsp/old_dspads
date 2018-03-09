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
#include "momo_adapter.h"
using namespace std;


extern map<string, vector<uint32_t> > inputadcat;
extern map<uint32_t, vector<string> > outputadcat;         // out
extern uint64_t g_logid_local, g_logid, g_logid_response;
extern map<string, uint16_t> dev_make_table;
extern uint64_t geodb;
extern int *numcount;
extern MD5_CTX hash_ctx;


// in,key-string
void transfer_adv_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &adcat = *((map<string, vector<uint32_t> > *)data);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key_start];
	cat.push_back(value);
}
// out,key-hex
void transfer_adv_hex(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<string> > &adcat = *((map<uint32_t, vector<string> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%x", &key);
	vector<string> &cat = adcat[key];
	cat.push_back(val);
}

uint64_t iab_to_uint64(string cat)
{
	cout << "cat = " << cat << endl;
	int icat = 0;
	sscanf(cat.c_str(), "%d", &icat);
	return icat;
}

void initialize_have_device(IN const com::immomo::moaservice::third::rtb::v12::BidRequest_Device &mobile, OUT COM_DEVICEOBJECT &device)
{
	device.ip = mobile.ip();
	device.ua = mobile.ua();
	if (device.ip != "")
	{
		device.location = getRegionCode(geodb, device.ip);
	}
	device.devicetype = DEVICE_PHONE;
	string s_make = mobile.make();
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
	device.osv = mobile.osv();

	//int32_t carrier = mobile.carrier_id();
	//if (carrier == 46000 || carrier == 46002 || carrier == 46007 || carrier == 46008 || carrier == 46020)
	//    device.carrier = CARRIER_CHINAMOBILE;
	//else if (carrier == 46001 || carrier == 46006 || carrier == 46009)
	//    device.carrier = CARRIER_CHINAUNICOM;
	//else if (carrier == 46003 || carrier == 46005 || carrier == 46011)
	//    device.carrier = CARRIER_CHINATELECOM;
	//else
	//	writeLog(g_logid_local,LOGDEBUG, "id :%s and device carrier :%d", bid.c_str(), carrier);

	if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::WIFI)
	{
		device.connectiontype = CONNECTION_WIFI;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_UNKNOWN)
	{
		device.connectiontype = CONNECTION_CELLULAR_UNKNOWN;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_2G)
	{
		device.connectiontype = CONNECTION_CELLULAR_2G;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_3G)
	{
		device.connectiontype = CONNECTION_CELLULAR_3G;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_4G)
	{
		device.connectiontype = CONNECTION_CELLULAR_4G;
	}

	if (!strcasecmp(mobile.os().c_str(), "Android"))
		device.os = OS_ANDROID;
	else if (!strcasecmp(mobile.os().c_str(), "iOS"))
		device.os = OS_IOS;

	if (mobile.macmd5() != "")
	{
		string lowid = mobile.macmd5();
		transform(lowid.begin(), lowid.end(), lowid.begin(), ::tolower);
		device.dids.insert(make_pair(DID_MAC_MD5, lowid));
	}

	if (mobile.did() != "")
	{
		if (device.os == OS_ANDROID)
			device.dids.insert(make_pair(DID_IMEI, mobile.did()));
		else if (device.os == OS_IOS)
			device.dpids.insert(make_pair(DPID_IDFA, mobile.did()));
	}

	if (mobile.didmd5() != "")
	{
		string lowid = mobile.didmd5();
		transform(lowid.begin(), lowid.end(), lowid.begin(), ::tolower);
		if (device.os == OS_ANDROID)
			device.dids.insert(make_pair(DID_IMEI_MD5, lowid));
		else if (device.os == OS_IOS)
			device.dpids.insert(make_pair(DPID_IDFA_MD5, lowid));
	}

	//     device.os = OS_IOS;                        // test
	//     device.dpidtype = DPID_IDFA;             // test
	//     device.dpid = "47b9caf8-2b0e-4398-8399-0a712a706bc4";    // test   

	if (mobile.has_geo())
	{
		device.geo.lat = mobile.geo().lat();
		device.geo.lon = mobile.geo().lon();
	}

	return;
}

void set_bcat(IN const com::immomo::moaservice::third::rtb::v12::BidRequest_Imp &imp, OUT set<uint32_t> &bcat)
{
	for (uint32_t i = 0; i < imp.bcat_size(); ++i)
	{
		string category = imp.bcat(i);
		vector<uint32_t> &adcat = inputadcat[category];
		for (uint32_t j = 0; j < adcat.size(); ++j)
			bcat.insert(adcat[j]);
	}
	return;
}

void set_badv(IN const com::immomo::moaservice::third::rtb::v12::BidRequest_Imp &imp, OUT set<string> &badv)
{
	for (uint32_t i = 0; i < imp.badv_size(); ++i)
	{
		badv.insert(imp.badv(i));
	}
	return;
}

static void initialize_native(IN int nativetype, OUT COM_NATIVE_REQUEST_OBJECT &com_native)
{
	com_native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
	com_native.plcmtcnt = 1;

	COM_ASSET_REQUEST_OBJECT asset_title;
	init_com_asset_request_object(asset_title);
	asset_title.required = 1;
	asset_title.id = 1;
	asset_title.type = NATIVE_ASSETTYPE_TITLE;
	asset_title.title.len = 9 * 3;
	com_native.assets.push_back(asset_title);

	COM_ASSET_REQUEST_OBJECT asset_data;
	init_com_asset_request_object(asset_data);
	asset_data.required = 1;
	asset_data.id = 2;
	asset_data.type = NATIVE_ASSETTYPE_DATA;
	asset_data.data.type = ASSET_DATATYPE_DESC;
	asset_data.data.len = 24 * 3;
	com_native.assets.push_back(asset_data);

	COM_ASSET_REQUEST_OBJECT asset_icon;
	init_com_asset_request_object(asset_icon);
	asset_icon.required = 1;
	asset_icon.id = 3;
	asset_icon.type = NATIVE_ASSETTYPE_IMAGE;
	asset_icon.img.type = ASSET_IMAGETYPE_ICON;
	asset_icon.img.w = 150;
	asset_icon.img.h = 150;
	com_native.assets.push_back(asset_icon);

	int imgsize = 0;
	if (nativetype == 1)
		imgsize = 1;
	else if (nativetype == 2)
		imgsize = 3;

	for (int i = 0; i < imgsize; ++i)
	{
		COM_ASSET_REQUEST_OBJECT com_asset;
		init_com_asset_request_object(com_asset);
		com_asset.id = 4;
		com_asset.required = 1;
		com_asset.type = NATIVE_ASSETTYPE_IMAGE;
		com_asset.img.type = ASSET_IMAGETYPE_MAIN;
		if (nativetype == 1)
		{
			com_asset.img.w = 690;
			com_asset.img.h = 345;
		}
		else
		{
			com_asset.img.w = 250;
			com_asset.img.h = 250;
		}
		com_native.assets.push_back(com_asset);
	}
}

// convert momo request to common request
int parse_common_request(IN com::immomo::moaservice::third::rtb::v12::BidRequest &bidrequest, 
	OUT COM_REQUEST &commrequest, OUT int &nativetype)
{
	int errorcode = E_SUCCESS;

	init_com_message_request(&commrequest);
	if (bidrequest.has_id())
	{
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

	// baidu now support one adzone
	writeLog(g_logid_local, LOGINFO, "bid=%s,imp_size=%d", bidrequest.id().c_str(), bidrequest.imp_size());
	for (uint32_t i = 0; i < bidrequest.imp_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;
		init_com_impression_object(&impressionobject);
		impressionobject.bidfloorcur = "CNY";

		const com::immomo::moaservice::third::rtb::v12::BidRequest_Imp &adslot = bidrequest.imp(i);
		impressionobject.id = adslot.id();
		impressionobject.bidfloor = adslot.bidfloor();
		set_bcat(adslot, commrequest.bcat);
		set_badv(adslot, commrequest.badv);

		writeLog(g_logid_local, LOGDEBUG, "bid=%s,pid=%s", bidrequest.id().c_str(), adslot.slotid().c_str());

		if (adslot.has_native()) // native
		{
			impressionobject.type = IMP_NATIVE;
			if (adslot.native().native_format_size() > 0)
			{
				bool threeflag = false;
				bool oneflag = false;
				for(uint32_t j = 0; j < adslot.native().native_format_size(); ++j)
				{
					if (adslot.native().native_format(j) == 2)
					{
						threeflag = true;
					}
					/////////check native one big pic//////////
                    if (adslot.native().native_format(j) == 1)
                    {   
                        oneflag = true;
                    }   
                    ////////check native one big pic///////////
				}

				///////////judge one big pic////////////////////
                if(oneflag == true)
                {   
                    nativetype = 1;
                    initialize_native(nativetype, impressionobject.native);
                }   
				//if (threeflag)
				//{
				//	nativetype = 2;
				//	initialize_native(nativetype, impressionobject.native);
				//}
				else
				{
					nativetype = adslot.native().native_format(0);
					if (nativetype != 1 && nativetype != 2 && nativetype != 3)
					{
						errorcode = E_REQUEST_IMP_TYPE;
					}
					else
					{
						initialize_native(nativetype, impressionobject.native);
					}
				}
			}
		}
		else
		{
			errorcode = E_REQUEST_IMP_TYPE;
		}
		commrequest.imp.push_back(impressionobject);
		break;
	}

	if (bidrequest.has_device())  // app bundle
	{
		const  com::immomo::moaservice::third::rtb::v12::BidRequest_Device &device = bidrequest.device();

		initialize_have_device(device, commrequest.device);

		if (commrequest.device.dids.size() == 0 && commrequest.device.dpids.size() == 0)
		{
			errorcode = E_REQUEST_DEVICE_ID;
			writeLog(g_logid_local, LOGERROR, "bid=%s dids and dpids size is zero!", bidrequest.id().c_str());
			goto release;
		}
	}
	else    // set device null
	{
		errorcode = E_REQUEST_DEVICE_ID;
		writeLog(g_logid_local, LOGERROR, "bid=%s have no mobile", bidrequest.id().c_str());
		goto release;
	}

	if (bidrequest.has_app())
	{
		commrequest.app.id = bidrequest.app().id();
		commrequest.app.name = bidrequest.app().name();
		commrequest.app.bundle = bidrequest.app().bundle();
		commrequest.app.cat.insert(0x90200);
	}

	if (bidrequest.has_user())
	{
		commrequest.user.id = bidrequest.user().id();
		int gender = bidrequest.user().gender();
		if (gender == 1)
		{
			commrequest.user.gender = GENDER_MALE;
		}
		else if (gender == 2)
		{
			commrequest.user.gender = GENDER_FEMALE;
		}
		commrequest.user.keywords = bidrequest.user().keywords();
		if (bidrequest.user().has_geo())
		{
			commrequest.user.geo.lat = bidrequest.user().geo().lat();
			commrequest.user.geo.lon = bidrequest.user().geo().lon();
		}
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
release:
	commrequest.cur.insert(string("CNY"));
badrequest:
	return errorcode;
}

static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, const int &advcat)
{
	if ((price - crequest.imp[0].bidfloor - 0.01) >= 0.000001)
		return true;

	return false;
}

// convert common response to response
void parse_common_response(IN COM_REQUEST &commrequest, IN COM_RESPONSE &commresponse, IN int nativetype, 
	IN ADXTEMPLATE &tanxmessage, OUT com::immomo::moaservice::third::rtb::v12::BidResponse &bidresponse)
{
	string trace_url_par = string("&") + get_trace_url_par(commrequest, commresponse);
	// replace result
	bidresponse.set_id(commresponse.id);

	for (uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = commresponse.seatbid[i];
		com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid *seat = bidresponse.add_seatbid();
		seat->set_seat(commresponse.bidid);
		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT &bidobject = seatbidobject.bid[j];

			com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid *bidresponseads = seat->add_bid();
			if (NULL == bidresponseads)
				continue;
			bidresponseads->set_id(commresponse.id);
			bidresponseads->set_impid(bidobject.impid);
			bidresponseads->set_price(bidobject.price);
			bidresponseads->set_adid(bidobject.adid);
			bidresponseads->set_crid(bidobject.mapid);

			set<uint32_t>::iterator pcat = bidobject.cat.begin();
			for (pcat; pcat != bidobject.cat.end(); ++pcat)
			{
				uint32_t category = *pcat;
				vector<string> &adcat = outputadcat[category];
				for (int k = 0; k < adcat.size(); ++k)
				{
					bidresponseads->add_cat(adcat[k]);
				}
			}
			//         string nurl = tanxmessage.nurl;
			//bidresponseads->set_nurl(nurl);

			// imp
			for (int k = 0; k < bidobject.imonitorurl.size(); ++k)
			{
				bidresponseads->add_imptrackers(bidobject.imonitorurl[k]);
			}

			for (int k = 0; k < tanxmessage.iurl.size(); ++k)
			{
				string iurl = tanxmessage.iurl[k] + trace_url_par;
				bidresponseads->add_imptrackers(iurl);
			}

			// clk
			for (int k = 0; k < bidobject.cmonitorurl.size(); ++k)
			{
				bidresponseads->add_clicktrackers(bidobject.cmonitorurl[k]);
			}
			//

			// dest url
			string curl = bidobject.curl;
			string cturl;
			for (uint32_t k = 0; k < tanxmessage.cturl.size(); ++k)
			{
				cturl = tanxmessage.cturl[k] + trace_url_par;
				if (bidobject.redirect)   // 重定向
				{
					int len = 0;
					string strcurl = curl;
					char *curl_c = urlencode(strcurl.c_str(), strcurl.size(), &len);
					cturl = cturl + "&curl=" + string(curl_c, len);
					free(curl_c);
				}
				break;
			}

			if (bidobject.redirect)  // redirect
			{
				curl = cturl;
				cturl = "";
			}

			if (cturl != "")
			{
				bidresponseads->add_clicktrackers(cturl);
			}

			com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid_NativeCreative *nativecr = bidresponseads->mutable_native_creative();
			if (nativetype == 1)
			{
				nativecr->set_native_format(com::immomo::moaservice::third::rtb::v12::FEED_LANDING_PAGE_LARGE_IMG);
			}
			else if (nativetype == 2)
			{
				nativecr->set_native_format(com::immomo::moaservice::third::rtb::v12::FEED_LANDING_PAGE_SMALL_IMG);
			}
			else if (nativetype == 3)
			{
				nativecr->set_native_format(com::immomo::moaservice::third::rtb::v12::NEARBY_LANDING_PAGE_NO_IMG);
			}

			(nativecr->mutable_landingpage_url())->set_url(curl);

			for (uint32_t k = 0; k < bidobject.native.assets.size(); ++k)
			{
				COM_ASSET_RESPONSE_OBJECT &asset = bidobject.native.assets[k];
				if (asset.type == NATIVE_ASSETTYPE_TITLE)
				{
					nativecr->set_title(asset.title.text);
				}
				else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
				{
					if (asset.img.type == ASSET_IMAGETYPE_ICON) // icon
					{
						com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid_NativeCreative_Image *logo = nativecr->mutable_logo();
						logo->set_url(asset.img.url);
					}
					else
					{
						com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid_NativeCreative_Image *image = nativecr->add_image();
						image->set_url(asset.img.url);
					}
				}
				else if (asset.type == NATIVE_ASSETTYPE_DATA)
				{
					nativecr->set_desc(asset.data.value);
				}
			}

		}
	}

	return;
}

void momo_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid)
{
	char *senddata = NULL;
	uint32_t sendlength = 0;
	com::immomo::moaservice::third::rtb::v12::BidRequest bidrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;

	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST commrequest;
	COM_RESPONSE commresponse;
	com::immomo::moaservice::third::rtb::v12::BidResponse bidresponse;
	int nativetype = 0;
	ADXTEMPLATE adxtemplate;
	struct timespec ts3, ts4;
	long lasttime2 = 0;
	string str_ferrcode = "";

	getTime(&ts3);

	if (recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
	{
		writeLog(g_logid_local, LOGERROR, "recv data is error!");
		err = E_BAD_REQUEST;
		goto release;
	}

	parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		va_cout("parse data from array failed!");
		err = E_BAD_REQUEST;
		goto release;
	}

	err = parse_common_request(bidrequest, commrequest, nativetype);

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
	ferrcode = bid_filter_response(ctx, index, commrequest, NULL, NULL, filter_bid_price, NULL, &commresponse, &adxtemplate);
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	flow_bid.set_bid_flow(ferrcode);

	err = ferrcode.errcode;
	g_ser_log.send_bid_log(index, commrequest, commresponse, err);

	if (err == E_SUCCESS)
	{
		parse_common_response(commrequest, commresponse, nativetype, adxtemplate, bidresponse);
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

		data = "Content-Type: application/x-protobuf\r\nContent-Length: " + 
			intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

		numcount[index]++;
		if (numcount[index] % 1000 == 0)
		{
			string str_response;
			google::protobuf::TextFormat::PrintToString(bidresponse, &str_response);
			writeLog(g_logid_local, LOGDEBUG, "response=%s", str_response.c_str());
			numcount[index] = 0;
		}
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d", commresponse.id.c_str(), 
			commresponse.seatbid[0].bid[0].mapid.c_str(), commrequest.device.deviceid.c_str(), 
			commrequest.device.deviceidtype);
		bidresponse.clear_seatbid();
	}

	getTime(&ts4);
	lasttime2 = (ts4.tv_sec - ts3.tv_sec) * 1000000 + (ts4.tv_nsec - ts3.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, bidrequest.id().c_str(), lasttime2, err, str_ferrcode.c_str());
	va_cout("%s,spent time:%ld,err:0x%x,desc:%s", bidrequest.id().c_str(), lasttime2, err, str_ferrcode.c_str());

release:
	if (err != E_SUCCESS)
	{
		data = "Status: 204 OK\r\n\r\n";
	}

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
