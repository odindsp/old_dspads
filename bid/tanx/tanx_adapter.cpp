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
#include "../../common/tanx/decode.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "tanx-bidding.pb.h"
#include "tanx_adapter.h"
using namespace std;


extern map<int, vector<uint32_t> > appcattable;
extern map< uint32_t, vector<int> > outputadcat;         // out
extern map<int, vector<uint32_t> > inputadcat;           // in
extern map<string, uint16_t> dev_make_table;
extern uint64_t g_logid_local, g_logid, g_logid_response;
extern MD5_CTX hash_ctx;
extern uint64_t geodb;
extern int *numcount;

string g_size[] = { "320x50", "240x290" };
set<string> setsize(g_size, g_size + 2);


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
	int icat = 0;
	sscanf(cat.c_str(), "%d", &icat);
	return icat;
}

// in,key-string
void transfer_adv_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<string> > &adcat = *((map<string, vector<string> > *)data);
	vector<string> &cat = adcat[key_start];
	cat.push_back(val);
}

bool is_banner(IN uint32_t viewtype)
{
	if (viewtype == 101 || viewtype == 102 || viewtype == 103)
		return true;
	else
		return false;
}

bool is_video(IN uint32_t viewtype)
{
	if (viewtype == 106 || viewtype == 107)
		return true;
	else
		return false;
}

bool is_native(IN uint32_t viewtype)
{
	if (viewtype == 108)   // native feeds
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

void initialize_have_device(OUT COM_DEVICEOBJECT &device, IN Tanx::BidRequest &bidrequest, IN const string &ip, IN const string &useragent)
{
	device.ip = ip;
	device.ua = useragent;

	if (device.ip != "")
	{
		device.location = getRegionCode(geodb, device.ip);
	}
	if (bidrequest.mobile().device().has_latitude())
		device.geo.lat = atof(bidrequest.mobile().device().latitude().c_str());
	if (bidrequest.mobile().device().has_longitude())
		device.geo.lon = atof(bidrequest.mobile().device().longitude().c_str());

	device.carrier = bidrequest.mobile().device().operator_();
	if (bidrequest.mobile().device().has_brand())
	{
		string s_make = bidrequest.mobile().device().brand();
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
	}

	device.model = bidrequest.mobile().device().model();
	device.osv = bidrequest.mobile().device().os_version();

	uint32_t conntype = 0;
	conntype = bidrequest.mobile().device().network();
	if (conntype >= 2)
		conntype += 1;
	device.connectiontype = conntype;

	string os;
	os = bidrequest.mobile().device().os();
	transform(os.begin(), os.end(), os.begin(), ::tolower);

	string platform;
	platform = bidrequest.mobile().device().platform();

	device.devicetype = DEVICE_UNKNOWN;
	if (platform.find("android") != string::npos || platform.find("iphone") != string::npos)
		device.devicetype = DEVICE_PHONE;
	else if (platform.find("ipad") != string::npos)
		device.devicetype = DEVICE_TABLET;
	else
	{
		string model = device.model;
		transform(model.begin(), model.end(), model.begin(), ::tolower);
		if (model.find("ipad") != string::npos)
		{
			device.devicetype = DEVICE_TABLET;
		}
		else if (model.find("iphone") != string::npos)
		{
			device.devicetype = DEVICE_PHONE;
		}
		else if (os.find("android") != string::npos)
		{
			device.devicetype = DEVICE_PHONE;
		}
	}

	// os
	int version = 0;
	if (device.osv != "")
		sscanf(device.osv.c_str(), "%d", &version);

	uchar deviceid[200] = "";
	bool hasid = false;
	if (bidrequest.mobile().device().has_device_id())
	{
		//   cout<<"exist device id"<<endl;
		string strdeviceid = bidrequest.mobile().device().device_id();
		if (strdeviceid.length() > 100)
		{
			writeLog(g_logid_local, LOGERROR, "deviceid = %s the length is too long ,it is not normal", strdeviceid.c_str());
		}
		else
		{
			decodeTanxDeviceId((char *)strdeviceid.c_str(), deviceid);
			if (strlen((char *)deviceid) == 0 || strcmp((char *)deviceid, "unknown") == 0)
			{
				//va_cout("decodde device id is unkonwn");
				writeLog(g_logid_local, LOGERROR, "deviceid = %s decode device id is unknown", strdeviceid.c_str());
			}
			else
				hasid = true;
		}
	}
	else
	{
		// va_cout("not exist device id");
		writeLog(g_logid_local, LOGERROR, "not exist device id");
	}

	//    device.didtype = DID_UNKNOWN;
	//    device.dpidtype = DPID_UNKNOWN;

	//if(!hasid)
	//  goto returnback;

	if (os.find("android") != string::npos)
	{
		device.os = OS_ANDROID;
		if (hasid)
		{
			device.dids.insert(make_pair(DID_IMEI, (char *)deviceid));
		}
	}
	else if (os.find("ios") != string::npos)
	{
		device.os = OS_IOS;
		if (hasid)
		{
			if (version < 6) //mac
			{
				device.dids.insert(make_pair(DID_MAC, (char *)deviceid));
			}
			else   // idfa
			{
				if (strlen((char *)deviceid) > 50)
				{
					writeLog(g_logid_local, LOGERROR, "decode deviceid  = %s IDFA is too long", deviceid);
				}
				else
				{
					device.dpids.insert(make_pair(DPID_IDFA, (char *)deviceid));
				}
			}
		}
	}
	else if (os.find("windows") != string::npos)
	{
		device.os = OS_WINDOWS;
	}
	else
		device.os = OS_UNKNOWN;

returnback:
	return;
}

void set_bcat(IN Tanx::BidRequest &bidrequest, OUT set<uint32_t> &bcat)
{
	for (uint32_t i = 0; i < bidrequest.excluded_ad_category_size(); ++i)
	{
		uint32_t category = bidrequest.excluded_ad_category(i);
		vector<uint32_t> &adcat = inputadcat[category];
		for (uint32_t j = 0; j < adcat.size(); ++j)
			bcat.insert(adcat[j]);
	}
	return;
}

void initialize_native_creative(OUT NATIVE_CREATIVE &nativecreative)
{
	nativecreative.title_max_safe_length = 0;
	nativecreative.ad_words_max_safe_length = 0;
	nativecreative.id = 0;
	nativecreative.creative_count = 0;
}

void initialize_title_or_data(IN vector<int> &fields, IN uint8_t required, IN int title_len, IN int ad_words_len, OUT COM_NATIVE_REQUEST_OBJECT &impnative)
{
	for (uint32_t i = 0; i < fields.size(); ++i)
	{
		if (fields[i] == 1)  // title
		{
			COM_ASSET_REQUEST_OBJECT asset;
			init_com_asset_request_object(asset);
			asset.id = 1;    // 1,title,2,icon,3,large image,4,description,5,rating
			asset.required = required;
			asset.type = NATIVE_ASSETTYPE_TITLE;
			if (title_len == 0)
				title_len = 15 * 3;
			//            cout<<"title_len="<<title_len<<endl;
			asset.title.len = title_len;
			impnative.assets.push_back(asset);
		}
		else if (fields[i] == 2)
		{
			COM_ASSET_REQUEST_OBJECT asset;
			init_com_asset_request_object(asset);
			asset.id = 4;    // 1,title,2,icon,3,large image,4,description,5,rating
			asset.required = required;
			asset.type = NATIVE_ASSETTYPE_DATA;
			asset.data.type = ASSET_DATATYPE_DESC;
			if (ad_words_len == 0)
				ad_words_len = 15 * 3;
			//            	 cout<<"ad_words_len="<<ad_words_len<<endl;
			asset.data.len = ad_words_len;
			impnative.assets.push_back(asset);
		}
	}
}

// 填充native信息
bool initialize_have_native(IN Tanx::BidRequest &bidrequest, OUT NATIVE_CREATIVE &nativecreative, OUT COM_NATIVE_REQUEST_OBJECT &impnative)
{
	if (bidrequest.has_mobile())
	{
		const Tanx::BidRequest_Mobile &mobile = bidrequest.mobile();
		// landing_type ,允许的创意打开方式，这个是否需要更改
		/*  set<int> c_type;
		int landing_type;
		for (uint32_t i = 0; i < mobile.landing_type_size();++i)
		{
		landing_type = mobile.landing_type(i);
		cflog(g_logid_local,LOGINFO, "bid=%s,landing_type=%d",bidrequest.bid().c_str(),landing_type);
		switch(landing_type)
		{
		case 0:
		c_type.insert(CTYPE_OPEN_DETAIL_PAGE);
		break;
		case 1:
		c_type.insert(CTYPE_DOWNLOAD_APP);
		break;
		case 2:
		c_type.insert(CTYPE_OPEN_WEBPAGE_WITH_WEBVIEW);
		break;
		case 3:
		c_type.insert(CTYPE_OPEN_WEBPAGE);
		break;
		case 4:
		c_type.insert(CTYPE_DOWNLOAD_APP_INTERNALLY_AND_ACTIVE_URL);
		break;
		case 91:
		c_type.insert(CTYPE_DOWNLOAD_APP_FROM_APPSTORE);
		break;
		default:
		break;
		}
		}

		for (uint8_t i = CTYPE_OPEN_WEBPAGE; i <= CTYPE_DOWNLOAD_APP_INTERNALLY_AND_ACTIVE_URL; ++i)
		{
		if (!c_type.count(i))
		impnative.bctype.insert(i);
		}
		*/


		// 获取native 属性
		string str_native_template_id;
		for (int id_size = 0; id_size < mobile.native_ad_template_size(); ++id_size)
		{
			const Tanx::BidRequest_Mobile_NativeAdTemplate &nativeadtemplate = mobile.native_ad_template(id_size);
			string native_template_id = nativeadtemplate.native_template_id();
			str_native_template_id = native_template_id;

			//		if (id_size == 0)
			{
				if (nativeadtemplate.areas_size() > 0)
				{
					const Tanx::BidRequest_Mobile_NativeAdTemplate_Area area = nativeadtemplate.areas(0);

					if (!area.has_creative())
						return false;

					//for (uint32_t i = 0;i < area.creative().action_fields_size(); ++i)
					//{
					//	vector<string> &tep_id = nativecreative.native_template_id[area.creative().action_fields(i)];
					//	tep_id.push_back(native_template_id);
					//}

					//if (area.creative().action_fields_size() == 0)
					//{
					//	vector<string> &tep_id = nativecreative.native_template_id[0];   // 非下载的写入0,下载的是1
					//	tep_id.push_back(native_template_id);
					//}

					int download_flag = 0;
					for (uint32_t i = 0; i < area.creative().required_fields_size(); ++i)
					{
						if (area.creative().required_fields(i) == 13)  // download
						{
							vector<string> &tep_id = nativecreative.native_template_id[1];
							tep_id.push_back(native_template_id);
							download_flag = 1;
							continue;
						}
						if (id_size == 0)
							nativecreative.required_fields.push_back(area.creative().required_fields(i));
					}
					if (download_flag == 0)
					{
						vector<string> &tep_id = nativecreative.native_template_id[0];
						tep_id.push_back(native_template_id);
					}

					if (id_size == 0)
					{
						nativecreative.id = area.id();
						nativecreative.creative_count = area.creative_count();
						//		if (area.has_creative())
						{

							for (uint32_t i = 0; i < area.creative().recommended_fields_size(); ++i)
							{
								nativecreative.recommended_fields.push_back(area.creative().recommended_fields(i));
							}

							nativecreative.title_max_safe_length = area.creative().title_max_safe_length();
							nativecreative.ad_words_max_safe_length = area.creative().ad_words_max_safe_length();
							nativecreative.image_size = area.creative().image_size();
						}
					}
				}
				else
					return false;
			}
		}

		if (nativecreative.native_template_id.size() == 0)
		{
			return false;
		}


		//set<string>::iterator p;
		//string str_native_template_id;

		//for (p = nativecreative.native_template_id.begin(); p != nativecreative.native_template_id.end(); ++p)
		//{
		//	str_native_template_id = *p;
		//	if (g_set_native_id.count(*p))  // 非下载
		//		continue;
		//	else
		//		break;
		//}

		//if (p == nativecreative.native_template_id.end())
		//{
		//	impnative.bctype.insert(CTYPE_DOWNLOAD_APP);
		//	impnative.bctype.insert(CTYPE_DOWNLOAD_APP_FROM_APPSTORE);
		//}

		if (!nativecreative.native_template_id.count(1))  // 不支持下载
		{
			impnative.bctype.insert(CTYPE_DOWNLOAD_APP);
			impnative.bctype.insert(CTYPE_DOWNLOAD_APP_FROM_APPSTORE);
		}

		{
			cflog(g_logid_local, LOGINFO, "bid=%s,native_template_id=%s", bidrequest.bid().c_str(), str_native_template_id.c_str());

			COM_ASSET_REQUEST_OBJECT asset;
			init_com_asset_request_object(asset);
			asset.id = 3;    // 1,title,2,icon,3,large image,4,description,5,rating
			asset.required = 1; // must have
			asset.type = NATIVE_ASSETTYPE_IMAGE;
			//              string native_size = native_cat[0];
			string native_size = nativecreative.image_size;
			cflog(g_logid_local, LOGINFO, "bid=%s,native_size=%s", bidrequest.bid().c_str(), native_size.c_str());
			sscanf(native_size.c_str(), "%dx%d", &asset.img.w, &asset.img.h);
			asset.img.type = ASSET_IMAGETYPE_MAIN;
			impnative.assets.push_back(asset);

			// 填充title/data
			initialize_title_or_data(nativecreative.required_fields, 1, nativecreative.title_max_safe_length, 
				nativecreative.ad_words_max_safe_length, impnative);
			initialize_title_or_data(nativecreative.recommended_fields, 0, nativecreative.title_max_safe_length, 
				nativecreative.ad_words_max_safe_length, impnative);
			cflog(g_logid_local, LOGINFO, "bid=%s,title_len=%d,ad_len=%d", bidrequest.bid().c_str(), 
				nativecreative.title_max_safe_length, nativecreative.ad_words_max_safe_length);
		}
	}
	return true;
}

// convert tanx request to common request
int parse_common_request(IN Tanx::BidRequest &bidrequest, OUT COM_REQUEST &commrequest, OUT Tanx::BidResponse &bidresponse, 
	OUT NATIVE_CREATIVE &nativecreative, OUT uint8_t &viewtype)
{
	int errorcode = E_SUCCESS;
	string ip, useragent;

	init_com_message_request(&commrequest);

	if (bidrequest.has_version())
		bidresponse.set_version(bidrequest.version());
	else
	{
		errorcode = E_BAD_REQUEST;
		goto badrequest;
	}

	if (bidrequest.has_bid())
	{
		bidresponse.set_bid(bidrequest.bid());
		commrequest.id = bidrequest.bid();

		if (bidrequest.bid().length() != 32)
		{
			writeLog(g_logid_local, LOGERROR, "id = %s the length is not equal 32", bidrequest.bid().c_str());
			errorcode = E_REQUEST_ID;
			goto badrequest;
		}
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

	if (bidrequest.has_ip())
		ip = bidrequest.ip();

	if (bidrequest.has_user_agent())
		useragent = bidrequest.user_agent();

	if (bidrequest.https_required())
		commrequest.is_secure = 1;
	else
		commrequest.is_secure = 0;

	// tanx new support one adzone
	for (uint32_t i = 0; i < bidrequest.adzinfo_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;
		init_com_impression_object(&impressionobject);
		impressionobject.bidfloorcur = "CNY";
		uint32_t id = 0;
		if (bidrequest.adzinfo(i).has_id())
		{
			id = bidrequest.adzinfo(i).id();
			char tempid[1024] = "";
			sprintf(tempid, "%u", id);
			impressionobject.id = tempid;
		}

		if (bidrequest.adzinfo(i).api_size() > 0)
			errorcode = E_REQUEST_IMP_TYPE;

		if (bidrequest.adzinfo(i).impression_repeatable())
			errorcode = E_REQUEST_IMP_TYPE;

		if (bidrequest.adzinfo(i).has_min_cpm_price())
			impressionobject.bidfloor = (double)(bidrequest.adzinfo(i).min_cpm_price() / 100.0);
		// get size eg:120x50
		int w = 0, h = 0;
		string size;
		if (bidrequest.adzinfo(i).has_size())
		{
			size = bidrequest.adzinfo(i).size();
			sscanf(size.c_str(), "%dx%d", &w, &h);
		}
		else
		{
			continue;
		}

		if (bidrequest.adzinfo(i).has_view_screen())
		{
			int adpos = bidrequest.adzinfo(i).view_screen();
			switch (adpos)
			{
			case 0:
				impressionobject.adpos = ADPOS_UNKNOWN;
				break;
			case 1:
				impressionobject.adpos = ADPOS_FIRST;
				break;
			case 2:
				impressionobject.adpos = ADPOS_SECOND;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
				impressionobject.adpos = ADPOS_OTHER;
				break;
			default:
				impressionobject.adpos = ADPOS_UNKNOWN;
				break;
			}
		}

		uint32_t viewtypetemp = 0;
		if (bidrequest.adzinfo(i).view_type_size() > 0)
			viewtypetemp = bidrequest.adzinfo(i).view_type(0);
		if (is_banner(viewtypetemp)) // banner  // viewtypetemp == 107
		{
			viewtype = IMP_BANNER;
			//        if (viewtypetemp == 107)   // video 现在先不处理，暂时忽略
			//            viewtype = IMP_VIDEO;
			impressionobject.type = IMP_BANNER;
			if (setsize.count(size))
			{
				w *= 2;
				h *= 2;
			}
			impressionobject.banner.w = w;
			impressionobject.banner.h = h;
			for (uint32_t j = 0; j < bidrequest.adzinfo(i).excluded_filter_size(); ++j)
			{
				uint32_t excludefilter = bidrequest.adzinfo(i).excluded_filter(j);
				if (1 == excludefilter)  // flash ad
					impressionobject.banner.btype.insert(ADTYPE_TEXT_LINK);
				else if (2 == excludefilter) // video ad
					impressionobject.banner.btype.insert(ADTYPE_IMAGE);
				else if (3 == excludefilter)
					impressionobject.banner.btype.insert(ADTYPE_FLASH);
				else if (4 == excludefilter)
					impressionobject.banner.btype.insert(ADTYPE_VIDEO);

			}
		}
		else
		{
			va_cout("other type");
			//  errorcode |= E_INVALID_PARAM;
			//     continue;
			if (is_video(viewtypetemp))  // video
			{
				errorcode = E_REQUEST_IMP_TYPE;
				//     continue;
				viewtype = IMP_VIDEO;
				impressionobject.type = IMP_VIDEO;
				impressionobject.video.w = w;
				impressionobject.video.h = h;

				for (uint8_t k = 1; k <= 4; ++k)
					impressionobject.video.mimes.insert(k);
				if (bidrequest.has_video())
				{
					// videoformat now is ignored
					impressionobject.video.minduration = (uint32_t)bidrequest.video().min_ad_duration();
					impressionobject.video.maxduration = (uint32_t)bidrequest.video().max_ad_duration();
					if (bidrequest.video().has_videoad_start_delay())
					{
						int delay = bidrequest.video().videoad_start_delay();
						if (delay == 0)
							impressionobject.video.display = DISPLAY_FRONT_PASTER;
						else if (delay == -1)
							impressionobject.video.display = DISPLAY_BACK_PASTER;
						else if (delay > 0)
							impressionobject.video.display = DISPLAY_MIDDLE_PASTER;
					}
				}

			}
			else if (is_native(viewtypetemp)) // now is ignored,don't match
			{
				//  continue;
				va_cout("it is native");
				writeLog(g_logid_local, LOGDEBUG, "id=%s,pid=%s", commrequest.id.c_str(), bidrequest.adzinfo(i).pid().c_str());
				viewtype = IMP_NATIVE;
				impressionobject.type = IMP_NATIVE;
				impressionobject.requestidlife = 10800;
				init_com_native_request_object(impressionobject.native);
				impressionobject.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
				impressionobject.native.plcmtcnt = 1;    // 目前填充广告数量1，tanx该字段不清楚，具体请求过来在修改
				if (!initialize_have_native(bidrequest, nativecreative, impressionobject.native)) // 没有广告模板id或者img_size
					errorcode = E_REQUEST_NATIVE;
			}
			else
				continue;
		}
		commrequest.imp.push_back(impressionobject);
	}


	if (bidrequest.has_mobile())  // app bundle
	{

		commrequest.app.bundle = bidrequest.mobile().package_name();
		commrequest.app.id = commrequest.app.bundle;
		commrequest.app.name = bidrequest.mobile().app_name();
		// app cat
		for (uint32_t j = 0; j < bidrequest.mobile().app_categories_size(); ++j)
		{
			int appid = bidrequest.mobile().app_categories(j).id();
			vector<uint32_t> &appcat = appcattable[appid];
			for (uint32_t i = 0; i < appcat.size(); ++i)
				commrequest.app.cat.insert(appcat[i]);
		}

		if (bidrequest.mobile().has_device()) // parse device
		{
			initialize_have_device(commrequest.device, bidrequest, ip, useragent);
			//  if (DID_UNKNOWN == commrequest.device.didtype && DPID_UNKNOWN == commrequest.device.dpidtype)
			if (commrequest.device.dids.size() == 0 && commrequest.device.dpids.size() == 0)
			{
				errorcode = E_REQUEST_DEVICE_ID;
				goto release;
			}

		}
		else    // don't have device
		{
			errorcode = E_REQUEST_DEVICE;
			//initialize_no_device(commrequest.device,ip,useragent);
			writeLog(g_logid_local, LOGERROR, "id = %s have no device", bidrequest.bid().c_str());
			goto release;
		}
	}
	else    // set device null
	{
		errorcode = E_REQUEST_DEVICE;
		// initialize_no_device(commrequest.device,ip,useragent);
		writeLog(g_logid_local, LOGERROR, "id = %s have no mobile", bidrequest.bid().c_str());
		goto release;
	}

	if (0 == commrequest.imp.size())  // 没有符合的曝光对象
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "id = %s imp size is zero!", bidrequest.bid().c_str());
		goto release;
	}

	if (commrequest.device.location == 0 || commrequest.device.location > 1156999999 || commrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			bidrequest.bid().c_str(), commrequest.device.ip.c_str(), commrequest.device.location);
		errorcode = E_REQUEST_DEVICE_IP;
	}

	MY_DEBUG
	release :
	set_bcat(bidrequest, commrequest.bcat);
	commrequest.cur.insert(string("CNY"));

badrequest:
	return errorcode;
}

int find_category(IN vector<int> sec)
{
	MY_DEBUG
		assert(sec.size() >= 1);
	if (sec.size() == 1)
	{
		return sec[0];
	}
	else
	{
		multiset<int> catset;
		for (uint32_t i = 0; i < sec.size(); ++i)
		{
			catset.insert(sec[i] / 100);
		}
		multiset<int>::iterator p = catset.begin();
		int maxcat = *p;
		int maxcount = 0;
		maxcount = catset.count(maxcat);
		for (uint32_t i = 0; i < maxcount; ++i)
			++p;
		while (p != catset.end())
		{
			int cattemp = *p;
			int count = catset.count(cattemp);
			if (count > maxcount)
			{
				maxcount = count;
				maxcat = cattemp;
			}
			for (uint32_t i = 0; i < count; ++i)
				++p;
		}
		return maxcat;
	}
	MY_DEBUG
}

// set response ad category
void set_response_category(IN set<uint32_t> &cat, OUT Tanx::BidResponse_Ads *bidresponseads)
{
	set<uint32_t>::iterator pcat = cat.begin();
	map< uint32_t, vector<int> > tanxadcategory;

	while (pcat != cat.end())  // common ad category
	{
		uint32_t category = *pcat;
		va_cout("cat response= %d", category);
		// exist key
		if (tanxadcategory.count(category))
		{
			vector<int> sec = tanxadcategory[category];
			int lastcat = find_category(sec);
			bidresponseads->add_category(lastcat);
		}
		else
		{
			uint32_t keycat = category & 0xFF00;
			if (tanxadcategory.count(keycat))
			{
				vector<int> sec = tanxadcategory[keycat];
				int lastcat = find_category(sec);
				bidresponseads->add_category(lastcat);
			}
			else
			{
				map< uint32_t, vector<int> >::iterator p = tanxadcategory.begin();
				while (p != tanxadcategory.end())
				{
					if ((p->first & 0xFF00) == keycat)
					{
						vector<int> sec = p->second;
						int lastcat = find_category(sec);
						bidresponseads->add_category(lastcat);
						break;
					}
					++p;
				}
			}
		}
		++pcat;
	}
	MY_DEBUG
		if (bidresponseads->category_size() == 0)
			bidresponseads->add_category(90199);
	return;
}

void set_response_category_new(IN set<uint32_t> &cat, OUT Tanx::BidResponse_Ads *bidresponseads)
{
	if (NULL == bidresponseads)
		return;

	set<uint32_t>::iterator pcat = cat.begin();

	for (pcat; pcat != cat.end(); ++pcat)
	{
		uint32_t category = *pcat;
		vector<int> &adcat = outputadcat[category];
		for (int j = 0; j < adcat.size(); ++j)
		{
			bidresponseads->add_category(adcat[j]);
		}

	}
	//    if (bidresponseads->category_size() == 0)
	//        bidresponseads->add_category(79901);
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
	cout << "adomain = " << bidobject.adomain << endl;
	cout << "w = " << bidobject.w << endl;
	cout << "h = " << bidobject.h << endl;
	cout << "curl = " << bidobject.curl << endl;
	cout << "monitorcode = " << bidobject.monitorcode << endl;
	for (uint32_t i = 0; i < bidobject.cmonitorurl.size(); ++i)
		cout << "cmonitorurl = " << bidobject.cmonitorurl[i] << endl;
	for (uint32_t i = 0; i < bidobject.imonitorurl.size(); ++i)
		cout << "imonitorurl = " << bidobject.imonitorurl[i] << endl;
	cout << "sourceurl = " << bidobject.sourceurl << endl;
	cout << "cid = " << bidobject.cid << endl;
	cout << "crid = " << bidobject.crid << endl;

	cout << "native" << endl;
	for (uint32_t i = 0; i < bidobject.native.assets.size(); ++i)
	{
		COM_ASSET_RESPONSE_OBJECT &asset = bidobject.native.assets[i];
		cout << "id = " << asset.id << endl;
		cout << "required =" << (int)asset.required << endl;
		cout << "type = " << (int)asset.type << endl;
		if (asset.type == NATIVE_ASSETTYPE_TITLE)
		{
			cout << "title.text = " << asset.title.text << endl;
		}
		else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
		{
			cout << "img.url = " << asset.img.url << endl;
			char buf[128];
			sprintf(buf, "%dx%d", asset.img.w, asset.img.h);
			cout << "img.size = " << buf << endl;
		}
		else if (asset.type == NATIVE_ASSETTYPE_DATA)
		{
			cout << "data.desc = " << asset.data.value << endl;
		}
	}

	return;
}
static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if ((price - crequest.imp[0].bidfloor - 0.01) >= 0.000001)
		return true;

	return false;
}

static bool filter_group_ext(IN const COM_REQUEST &crequest, IN const GROUPINFO_EXT &ext)
{
	if (ext.advid != "")
		return true;
	return false;
}

// convert common response to tanx response
void parse_common_response(IN COM_REQUEST &commrequest, IN COM_RESPONSE &commresponse, IN uint8_t viewtype, 
	IN ADXTEMPLATE &tanxmessage, IN NATIVE_CREATIVE &nativecreative, OUT Tanx::BidResponse &bidresponse)
{
#ifdef DEBUG
	cout << "id = " << commresponse.id << endl;
	cout << "bidid = " << commresponse.bidid << endl;
#endif
	string trace_url_par = string("&") + get_trace_url_par(commrequest, commresponse);
	// replace result
	int adcount = 0;
	for (uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT seatbidobject = commresponse.seatbid[i];
		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT bidobject = seatbidobject.bid[j];
			//replace_https_url(tanxmessage,commrequest.is_secure);

#ifdef DEBUG
			print_bid_object(bidobject);
#endif
			Tanx::BidResponse_Ads *bidresponseads = bidresponse.add_ads();
			if (NULL == bidresponseads)
				continue;

			int impid = atoi(bidobject.impid.c_str());
			bidresponseads->set_adzinfo_id(impid);

			double price = bidobject.price * 100;
			bidresponseads->set_max_cpm_price((uint32_t)(price));

			bidresponseads->set_creative_id(bidobject.mapid);

			bidresponseads->set_ad_bid_count_idx(adcount++);

			// set_response_category(bidobject.cat,bidresponseads);
			set_response_category_new(bidobject.cat, bidresponseads);
			if (bidobject.ctype == CTYPE_DOWNLOAD_APP || bidobject.ctype == CTYPE_DOWNLOAD_APP_FROM_APPSTORE)
				bidresponseads->add_category(901);

			if (bidresponseads->category_size() == 0)
				bidresponseads->add_category(79901);

			uint8_t creativetype = bidobject.type;
			if (creativetype == ADTYPE_TEXT_LINK || creativetype == ADTYPE_IMAGE)
			{
				bidresponseads->add_creative_type(creativetype);
			}
			else if (creativetype == ADTYPE_FLASH)
			{
				bidresponseads->add_creative_type(3);
			}
			else if (creativetype == ADTYPE_VIDEO)
			{
				bidresponseads->add_creative_type(4);
			}
			else if (creativetype == ADTYPE_FEEDS)
				bidresponseads->add_creative_type(2);

			//advertiser_ids,返回的广告主id
			if (bidobject.groupinfo_ext.advid == "")
			{
				va_cout("advid is empty!");
				writeLog(g_logid_local, LOGERROR, "bid=%s advid is empty", commresponse.id.c_str());
			}
			bidresponseads->add_advertiser_ids(atoi(bidobject.groupinfo_ext.advid.c_str()));

			// dest url
			string curl = bidobject.curl;

			va_cout("curl=%s", curl.c_str());
			string dest_url = curl;

			// click_through_url
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
			va_cout("cturl=%s", cturl.c_str());

			if (bidobject.redirect)  // redirect
			{
				curl = cturl;
				cturl = "";
			}

			// nurl
			string nurl = tanxmessage.nurl + trace_url_par;
			va_cout("nurl=%s", nurl.c_str());

			// iurl
			string iurl;
			if (tanxmessage.iurl.size() > 0)
			{
				iurl = tanxmessage.iurl[0] + trace_url_par;
			}
			va_cout("iurl=%s", iurl.c_str());

			if (viewtype == IMP_NATIVE) // native
			{
				bidresponseads->set_feedback_address(nurl);

				// 如果不提供resource_address则设置mobile_creative
				Tanx::MobileCreative *mobilecreative = bidresponseads->mutable_mobile_creative();
				mobilecreative->set_version(bidresponse.version());    // 目前先暂时填充为3，方便测试,以后修改为根据request字段填充
				mobilecreative->set_bid(commrequest.id);


				Tanx::MobileCreative_Creative *mobile_cr_creative = mobilecreative->add_creatives();

				for (uint32_t k = 0; k < bidobject.native.assets.size(); ++k)
				{
					COM_ASSET_RESPONSE_OBJECT &asset = bidobject.native.assets[k];
					if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						mobile_cr_creative->set_title(asset.title.text);
					}
					else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						mobile_cr_creative->set_img_url(asset.img.url);
						char buf[128];
						sprintf(buf, "%dx%d", asset.img.w, asset.img.h);
						cflog(g_logid_local, LOGINFO, "bid=%s,out_size=%s", commresponse.id.c_str(), buf);
						mobile_cr_creative->set_img_size(buf);
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						Tanx::MobileCreative_Creative_Attr *attr = mobile_cr_creative->add_attr();
						attr->set_name("ad_words");
						attr->set_value(asset.data.value);
					}
				}

				{
					if (bidobject.ctype == CTYPE_DOWNLOAD_APP || bidobject.ctype == CTYPE_DOWNLOAD_APP_FROM_APPSTORE) // 下载的创意
					{
						if (nativecreative.native_template_id.count(1))
						{
							Tanx::MobileCreative_Creative_Attr *attr = mobile_cr_creative->add_attr();
							attr->set_name("download_url");
							attr->set_value(curl);

							attr = mobile_cr_creative->add_attr();
							attr->set_name("download_type");
							if (commrequest.device.os == OS_IOS)
							{
								attr->set_value("3");
							}
							else if (commrequest.device.os == OS_ANDROID)
								attr->set_value("1");

							mobilecreative->set_native_template_id(nativecreative.native_template_id[1][0]);
						}
					}
					else
					{
						va_cout(" not download");
						if (nativecreative.native_template_id.count(0))
						{
							cout << nativecreative.native_template_id[0][0] << endl;
							mobilecreative->set_native_template_id(nativecreative.native_template_id[0][0]);
							mobile_cr_creative->set_click_url(curl);

							Tanx::MobileCreative_Creative_Attr *attr = mobile_cr_creative->add_attr();
							attr->set_name("open_type");
							attr->set_value("2");
						}
					}
				}

				mobile_cr_creative->set_destination_url(dest_url);    // native不需要替换，上面已经替换
				mobile_cr_creative->set_creative_id(bidobject.mapid);

				Tanx::MobileCreative_Creative_TrackingEvents *track = mobile_cr_creative->mutable_tracking_events();
				track->add_impression_event(iurl);

				for (uint32_t k = 0; k < bidobject.imonitorurl.size(); ++k)
				{
					string moncode = bidobject.imonitorurl[k];
					track->add_impression_event(moncode);
				}

				if (cturl != "") // 没有重定向，将自己的地址放到点击监控字段。
				{
					track->add_click_event(cturl);
					//	track->add_download_complete_event(cturl);  // test
				}


				for (uint32_t k = 0; k < bidobject.cmonitorurl.size(); ++k)
				{
					string moncode = bidobject.cmonitorurl[k];
					track->add_click_event(moncode);
				}
			}
			else
			{
				bidresponseads->set_feedback_address(nurl);
				bidresponseads->add_destination_url(dest_url);
				bidresponseads->add_click_through_url(dest_url);

				//video snippet
				uint8_t os = commrequest.device.os;
				string admurl;
				for (uint32_t k = 0; k < tanxmessage.adms.size(); ++k)
				{
					if (tanxmessage.adms[k].os == os && tanxmessage.adms[k].type == creativetype)
					{
						admurl = tanxmessage.adms[k].adm;
						break;
					}
				}

				if (admurl.length() > 0)
				{
					replaceMacro(admurl, "#SOURCEURL#", bidobject.sourceurl.c_str());
					replaceMacro(admurl, "#CURL#", curl.c_str());

					string moncode = bidobject.monitorcode;
					replaceMacro(admurl, "#MONITORCODE#", moncode.c_str());
					//		replaceMacro(admurl,"#NURL#",nurl.c_str());
					replaceMacro(admurl, "#IURL#", iurl.c_str());

					if (bidobject.cmonitorurl.size() > 0)
					{
						string moncode = bidobject.cmonitorurl[0];
						replaceMacro(admurl, "#CMONITORURL#", moncode.c_str());
					}
					if (cturl != "")
					{
						replaceMacro(admurl, "#CTURL#", cturl.c_str());
					}

					if (viewtype == IMP_VIDEO) //video
						bidresponseads->set_video_snippet(admurl);
					else
					{
						MY_DEBUG
							bidresponseads->set_html_snippet(admurl);
					}
				}
				else
				{
					va_cout("tanx adm is empty!");
					cflog(g_logid_local, LOGERROR, "tanx adm is empty! id = %s,os = %d,type = %d", commrequest.id.c_str(), os, creativetype);
				}
			}
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

void convert_response_to_json(IN Tanx::BidResponse &bidresponse, OUT string &response_out)
{
	char *text = NULL;

	json_t *root = NULL;

	root = json_new_object();
	if (root == NULL)
		return;
	jsonInsertString(root, "bid", bidresponse.bid().c_str());
	jsonInsertInt(root, "version", bidresponse.version());
	json_t *array_ads = json_new_array();
	for (uint32_t i = 0; i < bidresponse.ads().size(); ++i)
	{
		const Tanx::BidResponse_Ads &ads = bidresponse.ads(i);
		json_t *label_ads = json_new_object();

		jsonInsertInt(label_ads, "adzinfo_id", ads.adzinfo_id());
		jsonInsertInt(label_ads, "max_cpm_price", ads.max_cpm_price());
		jsonInsertInt(label_ads, "ad_bid_count_idx", ads.ad_bid_count_idx());
		jsonInsertString(label_ads, "creative_id", ads.creative_id().c_str());

		// category
		json_t *array_category = json_new_array();
		json_t *value_catrgory;
		for (uint32_t j = 0; j < ads.category().size(); ++j)
		{
			char catrgory[128];
			sprintf(catrgory, "%d", ads.category(j));
			value_catrgory = json_new_number(catrgory);
			json_insert_child(array_category, value_catrgory);
		}
		json_insert_pair_into_object(label_ads, "category", array_category);

		//creative_type
		json_t *array_creative_type = json_new_array();
		json_t *value_creative_type;
		for (uint32_t j = 0; j < ads.creative_type().size(); ++j)
		{
			char creative_type[128];
			sprintf(creative_type, "%d", ads.creative_type(j));
			value_creative_type = json_new_number(creative_type);
			json_insert_child(array_creative_type, value_creative_type);
		}
		json_insert_pair_into_object(label_ads, "creative_type", array_creative_type);

		//feedback_address
		jsonInsertString(label_ads, "feedback_address", ads.feedback_address().c_str());
		//advertiser_ids
		json_t *array_advertiser_ids = json_new_array();
		json_t *value_advertiser_ids;
		for (uint32_t j = 0; j < ads.advertiser_ids().size(); ++j)
		{
			char advertiser_ids[128];
			sprintf(advertiser_ids, "%d", ads.advertiser_ids(j));
			value_advertiser_ids = json_new_number(advertiser_ids);
			json_insert_child(array_advertiser_ids, value_advertiser_ids);
		}
		json_insert_pair_into_object(label_ads, "advertiser_ids", array_advertiser_ids);

		// mobile_creative
		if (ads.has_mobile_creative())
		{
			const Tanx::MobileCreative &mobilecreative = ads.mobile_creative();
			json_t *label_mobile_creative = json_new_object();

			jsonInsertString(label_mobile_creative, "bid", mobilecreative.bid().c_str());
			jsonInsertInt(label_mobile_creative, "version", mobilecreative.version());
			//native_template_id
			jsonInsertString(label_mobile_creative, "native_template_id", mobilecreative.native_template_id().c_str());

			// creatives
			json_t *array_creatives = json_new_array();
			json_t *label_mobile_creative_cr;
			for (uint32_t j = 0; j < mobilecreative.creatives().size(); ++j)
			{
				const Tanx::MobileCreative_Creative &creative = mobilecreative.creatives(j);
				// img_url
				label_mobile_creative_cr = json_new_object();
				jsonInsertString(label_mobile_creative_cr, "img_url", creative.img_url().c_str());
				//img_size
				jsonInsertString(label_mobile_creative_cr, "img_size", creative.img_size().c_str());
				//title
				jsonInsertString(label_mobile_creative_cr, "title", creative.title().c_str());
				//click_url
				//                 va_cout("click_url=%s",creative.click_url().c_str());
				jsonInsertString(label_mobile_creative_cr, "click_url", creative.click_url().c_str());
				//destination_url
				//                va_cout("destination_url=%s",creative.destination_url().c_str());
				jsonInsertString(label_mobile_creative_cr, "destination_url", creative.destination_url().c_str());
				//creative_id
				jsonInsertString(label_mobile_creative_cr, "creative_id", creative.creative_id().c_str());

				//attr
				json_t *array_attr = json_new_array();
				json_t *label_attr;
				for (uint32_t k = 0; k < creative.attr().size(); ++k)
				{
					const Tanx::MobileCreative_Creative_Attr &attr = creative.attr(k);
					label_attr = json_new_object();
					//name
					jsonInsertString(label_attr, "name", attr.name().c_str());
					//value
					jsonInsertString(label_attr, "value", attr.value().c_str());
					json_insert_child(array_attr, label_attr);
				}
				json_insert_pair_into_object(label_mobile_creative_cr, "attr", array_attr);

				//tracking_events
				if (creative.has_tracking_events())
				{
					const Tanx::MobileCreative_Creative_TrackingEvents	&event = creative.tracking_events();
					json_t *label_event = json_new_object();
					//impression_event
					json_t *array_impression_event = json_new_array();
					json_t *value_impression_event;
					for (uint32_t k = 0; k < event.impression_event().size(); ++k)
					{
						value_impression_event = json_new_string(event.impression_event(k).c_str());
						json_insert_child(array_impression_event, value_impression_event);
					}
					json_insert_pair_into_object(label_event, "impression_event", array_impression_event);

					//click_event
					json_t *array_click_event = json_new_array();
					json_t *value_click_event;
					for (uint32_t k = 0; k < event.click_event().size(); ++k)
					{
						value_click_event = json_new_string(event.click_event(k).c_str());
						json_insert_child(array_click_event, value_click_event);
					}
					json_insert_pair_into_object(label_event, "click_event", array_click_event);
					//
					// test add_download_complete_event
					//
					json_t *array_download_complete_event = json_new_array();
					json_t *value_download_complete_event;
					for (uint32_t k = 0; k < event.download_complete_event().size(); ++k)
					{
						value_download_complete_event = json_new_string(event.download_complete_event(k).c_str());
						json_insert_child(array_download_complete_event, value_download_complete_event);
					}
					json_insert_pair_into_object(label_event, "download_complete_event", array_download_complete_event);

					json_insert_pair_into_object(label_mobile_creative_cr, "tracking_events", label_event);
				}

				json_insert_child(array_creatives, label_mobile_creative_cr);
			}
			json_insert_pair_into_object(label_mobile_creative, "creatives", array_creatives);

			json_insert_pair_into_object(label_ads, "mobile_creative", label_mobile_creative);
		}


		json_insert_child(array_ads, label_ads);
	}
	json_insert_pair_into_object(root, "ads", array_ads);

	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		response_out = text;
		free(text);
		text = NULL;
	}

	json_free_value(&root);

	return;
}

void tanx_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid)
{
	char *senddata = NULL;
	uint32_t sendlength = 0;
	Tanx::BidRequest bidrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;

	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST commrequest;
	COM_RESPONSE commresponse;
	Tanx::BidResponse bidresponse;
	uint8_t viewtype = 0;
	ADXTEMPLATE adxtemplate;
	struct timespec ts3, ts4;
	long lasttime2 = 0;
	string str_ferrcode = "";
	NATIVE_CREATIVE nativecreative;
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

	initialize_native_creative(nativecreative);
	err = parse_common_request(bidrequest, commrequest, bidresponse, nativecreative, viewtype);

	if (err == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGERROR, "the request format is error!");
		va_cout("it is a wrong request format");
		goto release;
	}
	else if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "id = %s the request is not suit for bidding!", bidrequest.bid().c_str());
		va_cout("the request is not suit for bidding!");
		goto returnresponse;
	}

	flow_bid.set_req(commrequest);
	ferrcode = bid_filter_response(ctx, index, commrequest, 
		NULL, filter_group_ext, filter_bid_price, NULL, 
		&commresponse, &adxtemplate);
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	flow_bid.set_bid_flow(ferrcode);
	err = ferrcode.errcode;

	g_ser_log.send_bid_log(index, commrequest, commresponse, err);

	if (err == E_SUCCESS)
	{
		parse_common_response(commrequest, commresponse, viewtype, adxtemplate, nativecreative, bidresponse);
		va_cout("parse common response to tanx response success!");
	}

returnresponse:
	if (err == E_REDIS_CONNECT_INVALID)
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
	data = "Content-Length: " + intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

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

	bidresponse.clear_ads();
	getTime(&ts4);
	lasttime2 = (ts4.tv_sec - ts3.tv_sec) * 1000000 + (ts4.tv_nsec - ts3.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, bidrequest.bid().c_str(), lasttime2, err, str_ferrcode.c_str());
	va_cout("%s,spent time:%ld,err:0x%x,desc:%s", bidrequest.bid().c_str(), lasttime2, err, str_ferrcode.c_str());

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
