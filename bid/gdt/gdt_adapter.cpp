#include <syslog.h>
#include <signal.h>
#include <map>
#include <vector>
#include <string>
#include <errno.h>
#include <algorithm>
#include <set>
#include <google/protobuf/text_format.h>
#include "gdt_rtb.pb.h"
#include "../../common/bid_filter.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "gdt_adapter.h"
using namespace std;

extern map<int64, vector<uint32_t> > inputadcat;           // in
extern map<int, vector<uint32_t> > appcattable;
extern map<int32, string> advid_table;
extern uint64_t g_logid_local, g_logid, g_logid_response;
extern map<string, uint16_t> dev_make_table;

extern uint64_t geodb;
extern MD5_CTX hash_ctx;
extern int *numcount;
#define IMP_MAXNUM 1


struct ProiVal
{
	int index;
	int proi;
	bool operator < (const ProiVal &ref_)const{ return proi < ref_.proi; }
	bool operator == (const ProiVal &ref_)const{ return proi == ref_.proi; }
};

vector<int> cal_random_index(const vector<COM_IMPRESSIONOBJECT> & imps)
{
	vector<ProiVal> cal_ret;
	for (uint32 i = 0; i < imps.size(); i++)
	{
		ProiVal val = { i, rand() % imps[i].ext.adv_priority };
		cal_ret.push_back(val);
	}

	sort(cal_ret.begin(), cal_ret.end());

	vector<int> ret;
	for (uint32 i = 0; i < cal_ret.size(); i++)
	{
		ret.push_back(cal_ret[i].index);
	}

	reverse(ret.begin(), ret.end());
	return ret;
}

// in,key-int
void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int64, vector<uint32_t> > &adcat = *((map<int64, vector<uint32_t> > *)data);
	int64 key = 0;
	sscanf(key_start.c_str(), "%ld", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}

void transfer_advid_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int32, string> &advid = *((map<int32, string> *)data);
	int32 key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	string value = val;
	advid.insert(pair<int32, string>(key, value));
}

void set_bcat(IN gdt::adx::BidRequest::Impression impressions, OUT set<uint32_t> &bcat)
{
	for (uint32_t i = 0; i < impressions.blocking_industry_id_size(); ++i)
	{
		int64 category = impressions.blocking_industry_id(i);
		map<int64, vector<uint32_t> >::iterator cat = inputadcat.find(category);
		if (cat != inputadcat.end())
		{
			vector<uint32_t> &adcat = cat->second;
			for (uint32_t j = 0; j < adcat.size(); ++j)
				bcat.insert(adcat[j]);
		}
	}
	return;
}

void set_wadv(IN gdt::adx::BidRequest::Impression impressions, OUT set<string> &bcat)
{
	for (uint32_t i = 0; i < impressions.advertiser_whitelist_size(); ++i)
	{
		string category = impressions.advertiser_whitelist(i);
		if (category != "")
		{
			bcat.insert(category);
		}
	}
	return;
}

void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi)
{
	proi = 100;
	string temp = impInfo;
	string typestr;
	int pos_type = temp.find(";");
	if (pos_type != string::npos)
	{
		typestr = temp.substr(0, pos_type);
		int pos_type1 = typestr.find(":");
		if (pos_type1 != string::npos)
		{
			typestr = typestr.substr(pos_type1 + 1);
			advtype = atoi(typestr.c_str());
			impInfo = temp.substr(pos_type + 1);
			cout << "advtype is : " << advtype << endl;
		}
		if (advtype == 1)
		{
			get_banner_info(impInfo, w, h, proi);
		}
		else
		{
			get_native_info(impInfo, w, h, tLen, dLen, proi);
		}
	}
	return;
}

void get_banner_info(string impInfo, uint32 &w, uint32 &h, uint32 &proi)
{
	string temp = impInfo;
	string wstr, hstr;
	int pos_w_h = temp.find(";");
	if (pos_w_h != string::npos)
	{
		wstr = temp.substr(0, pos_w_h);
		int pos_w = wstr.find(":");
		if (pos_w != string::npos)
		{
			wstr = wstr.substr(pos_w + 1);
			w = atoi(wstr.c_str());
		}

		temp = temp.substr(pos_w_h + 1);
		int pos_h = temp.find(":");
		if (pos_h != string::npos)
		{
			hstr = temp.substr(pos_h + 1);
			h = atoi(hstr.c_str());
		}
	}

	int pos_p = impInfo.find("proi:");
	if (pos_p != string::npos)
	{
		string pstr = impInfo.substr(pos_p + 5);
		proi = atoi(pstr.c_str());
	}

	return;
}

void get_native_info(string impInfo, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi)
{
	string temp = impInfo;
	string tempInfo, tagStr, infoStr;
	int pos_str, pos_info, pos_next;
	while (temp != "")
	{
		pos_str = temp.find(";");
		pos_next = temp.find(":");
		if (pos_str != string::npos || pos_next != string::npos)
		{
			if (pos_str != string::npos)
			{
				tempInfo = temp.substr(0, pos_str);
			}
			else
			{
				tempInfo = temp;
			}
			pos_info = tempInfo.find(":");
			if (pos_info != string::npos)
			{
				tagStr = tempInfo.substr(0, pos_info);
				infoStr = tempInfo.substr(pos_info + 1);
				if (tagStr == "w")
				{
					w = atoi(infoStr.c_str());
				}
				else if (tagStr == "h")
				{
					h = atoi(infoStr.c_str());
				}
				else if (tagStr == "title")
				{
					tLen = atoi(infoStr.c_str()) * 3;
				}
				else if (tagStr == "des")
				{
					dLen = atoi(infoStr.c_str()) * 3;
				}
				else if (tagStr == "proi")
				{
					proi = atoi(infoStr.c_str());
				}
				if (pos_str != string::npos)
				{
					temp = temp.substr(pos_str + 1);
				}
				else
					break;
			}
		}
		else
		{
			break;
		}
	}

	return;
}

static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio,
							 IN const double &price, IN const int &advcat)
{
	if ((price - crequest.imp[0].bidfloor - 0.01) >= 0.000001)
		return true;

	return false;
}

static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &ext)
{
	bool filter = true;

	string creativ_spec;
	int creativ_id = 0;
	json_t *root, *label;
	root = label = NULL;

	if (ext.id != "")
	{
		if (crequest.imp[0].ext.advid != 0)
		{
			json_parse_document(&root, ext.ext.c_str());

			if (root == NULL)
			{
				filter = false;
			}
			else
			{
				if (JSON_FIND_LABEL_AND_CHECK(root, label, "creative_spec", JSON_NUMBER))
					creativ_spec = label->child->text;
				creativ_id = atoi(creativ_spec.c_str());
				//cout <<"creative_spec is :"<< creativ_spec.c_str() <<endl;
				json_free_value(&root);
			}

			//		   if(creativ_id != crequest.imp[0].ext.advid)
			//		   {
			//			   filter = false;
			//		   }
		}
	}
	else
	{
		filter = false;
	}


	return filter;
}

static bool filter_group_ext(IN const COM_REQUEST &crequest, IN const GROUPINFO_EXT &ext)
{
	if (ext.advid != "")
	{
		if (!crequest.wadv.empty())
		{
			set<string>::iterator iter;
			iter = crequest.wadv.find(ext.advid);
			if (iter != crequest.wadv.end())
			{
				return true;
			}
			return false;
		}
		return true;
	}

	return false;
}

void initialize_have_device(OUT COM_DEVICEOBJECT &device, IN const gdt::adx::BidRequest::Device &reqdevice,
							IN const gdt::adx::BidRequest &adrequest)
{
	if (reqdevice.has_device_type())
	{
		int devicetype = reqdevice.device_type();
		if (devicetype == gdt::adx::BidRequest::DEVICETYPE_MOBILE)
		{
			device.devicetype = DEVICE_PHONE;
		}
		else if (devicetype == gdt::adx::BidRequest::DEVICETYPE_PAD)
		{
			device.devicetype = DEVICE_TABLET;
		}
		else
		{
			device.devicetype = DEVICE_UNKNOWN;
		}
		writeLog(g_logid_local, LOGINFO, "bid=%s, devicetype=%d ", adrequest.id().c_str(), reqdevice.device_type());
	}
	if (reqdevice.has_user_agent())
	{
		if (reqdevice.user_agent() != "")
		{
			device.ua = reqdevice.user_agent();
		}
	}
	if (reqdevice.has_os())
	{
		int deviceOStype = reqdevice.os();
		if (deviceOStype == gdt::adx::BidRequest::OS_ANDROID)
		{
			device.os = OS_ANDROID;
			if (reqdevice.android_id() != "")
			{
				device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, reqdevice.android_id()));
			}
		}
		else if (deviceOStype == gdt::adx::BidRequest::OS_IOS)
		{
			device.os = OS_IOS;
			if (reqdevice.idfa() != "")
			{
				device.dpids.insert(make_pair(DPID_IDFA, reqdevice.idfa()));
			}
		}
		else if (deviceOStype == gdt::adx::BidRequest::OS_WINDOWS)
		{
			device.os = OS_WINDOWS;
		}
		else
		{
			device.os = OS_UNKNOWN;
		}
	}
	if (reqdevice.has_carrier())
	{
		int carrier = reqdevice.carrier();
		if (carrier == gdt::adx::BidRequest::CARRIER_CHINAMOBILE)
		{
			device.carrier = CARRIER_CHINAMOBILE;
		}
		else if (carrier == gdt::adx::BidRequest::CARRIER_CHINATELECOM)
		{
			device.carrier = CARRIER_CHINATELECOM;
		}
		else if (carrier == gdt::adx::BidRequest::CARRIER_CHINAUNICOM)
		{
			device.carrier = CARRIER_CHINAUNICOM;
		}
		else
		{
			device.carrier = CARRIER_UNKNOWN;
		}
	}
	if (reqdevice.has_connection_type())
	{
		int connecttype = reqdevice.connection_type();
		if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_2G)
		{
			device.connectiontype = CONNECTION_CELLULAR_2G;
		}
		else if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_3G)
		{
			device.connectiontype = CONNECTION_CELLULAR_3G;
		}
		else if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_4G)
		{
			device.connectiontype = CONNECTION_CELLULAR_4G;
		}
		else if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_WIFI)
		{
			device.connectiontype = CONNECTION_WIFI;
		}
		else
		{
			device.connectiontype = CONNECTION_UNKNOWN;
		}
	}
	if (reqdevice.has_os_version())
	{
		device.osv = reqdevice.os_version();
	}

	if (reqdevice.has_brand_and_model())
	{
		device.model = reqdevice.brand_and_model();
	}

	string s_make = reqdevice.manufacturer();
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;
		device.makestr = reqdevice.manufacturer();
		for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				device.make = it->second;
				break;
			}
		}
	}
	if (adrequest.has_ip())
	{
		device.ip = adrequest.ip();
		if (device.ip.size() > 0)
		{
			int location = getRegionCode(geodb, device.ip);
			device.location = location;
		}
	}
	if (adrequest.has_geo())
	{
		device.geo.lat = (double)adrequest.geo().latitude();
		device.geo.lon = (double)adrequest.geo().longitude();
	}

	return;
}

int set_native(COM_NATIVE_REQUEST_OBJECT &native, const gdt::adx::BidRequest::Impression &impressions,
	int w, int h, int tLen, int dLen, int32 advid)
{
	native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;

	if (impressions.natives().size() != 0)
	{
		for (int j = 0; j < IMP_MAXNUM; ++j)
		{
			int request_field = impressions.natives(j).required_fields();
			int flag = 0;
			flag = request_field & 0x01;
			COM_ASSET_REQUEST_OBJECT asset;
			if (flag)
			{
				asset.id = j;
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_TITLE;
				asset.title.len = tLen;
				native.assets.push_back(asset);
			}
			flag = request_field & 0x02;
			if (flag)
			{
				asset.id = j; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_IMAGE;
				asset.img.type = ASSET_IMAGETYPE_ICON;
				if (advid == 147 || advid == 148 || advid == 149 || advid == 150)
				{
					asset.img.w = 300;
					asset.img.h = 300;
				}
				if (advid == 65)
				{
					asset.img.w = 80;
					asset.img.h = 80;
				}
				native.assets.push_back(asset);
			}
			flag = request_field & 0x08;
			if (flag)
			{
				asset.id = j; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_DATA;
				asset.data.type = 2;
				asset.data.len = dLen;
				native.assets.push_back(asset);
			}
			flag = request_field & 0x04;
			if (flag)
			{
				asset.id = j; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_IMAGE;
				asset.img.type = ASSET_IMAGETYPE_MAIN;
				asset.img.w = w;
				asset.img.h = h;
				native.assets.push_back(asset);
			}
		}
	}
	else
	{
		COM_ASSET_REQUEST_OBJECT asset;

		if (w != 0 && h != 0)
		{
			asset.id = 1; // must have
			asset.required = 1;
			asset.type = NATIVE_ASSETTYPE_IMAGE;
			asset.img.type = ASSET_IMAGETYPE_MAIN;
			asset.img.w = w;
			asset.img.h = h;
			native.assets.push_back(asset);
		}
		if (tLen != 0)
		{
			asset.id = 2;
			asset.required = 1;
			asset.type = NATIVE_ASSETTYPE_TITLE;
			asset.title.len = tLen;
			native.assets.push_back(asset);
		}
		if (dLen != 0)
		{
			asset.id = 3; // must have
			asset.required = 1;
			asset.type = NATIVE_ASSETTYPE_DATA;
			asset.data.type = 2;
			asset.data.len = dLen;
			native.assets.push_back(asset);
		}
	}
}

// convert gdt request to common request
int parse_common_request(IN gdt::adx::BidRequest &adrequest, OUT COM_REQUEST &commrequest, 
						 OUT gdt::adx::BidResponse &adresponse, OUT string &data)
{
	int errorcode = E_SUCCESS;
	uint32 w = 0;
	uint32 h = 0;
	uint32 tLen = 0;
	uint32 dLen = 0;
	uint32 proi = 0;
	string responseret;

#ifdef DEBUG
	google::protobuf::TextFormat::PrintToString(adrequest, &responseret);
	writeLog(g_logid_local, LOGINFO, "request is : %s ", responseret.c_str());
	cout << "request is : " << responseret.c_str() << endl;
#endif

	init_com_message_request(&commrequest);
	if (adrequest.has_id())
	{
		adresponse.set_request_id(adrequest.id());
		commrequest.id = adrequest.id();
	}
	else
	{
		errorcode = E_REQUEST_NO_BID;
		goto badrequest;
	}

	if (adrequest.is_ping())
	{
		errorcode = E_REQUEST_IS_PING;
		goto badrequest;
	}

	if (adrequest.has_area_code())
	{
		writeLog(g_logid_local, LOGINFO, "bid=%s, area_code=%d ", adrequest.id().c_str(), adrequest.area_code());
	}
	if (adrequest.has_device())
	{
		const gdt::adx::BidRequest::Device &device = adrequest.device();
		initialize_have_device(commrequest.device, device, adrequest);
	}

	commrequest.support_deep_link = adrequest.support_deep_link() ? 1 : 0;

	for (int i = 0; i < adrequest.impressions_size(); ++i)
	{
		//imp
		COM_IMPRESSIONOBJECT impressionobject;
		init_com_impression_object(&impressionobject);

		const gdt::adx::BidRequest::Impression &impressions = adrequest.impressions(i);
		impressionobject.id = impressions.id();
		impressionobject.bidfloorcur = "CNY";
		impressionobject.bidfloor = (double)(impressions.bid_floor() / 100);

		set_bcat(impressions, commrequest.bcat);
		set_wadv(impressions, commrequest.wadv);
		int32 advid = 0;
		map<int32, string>::iterator iter;
		int32 advtype = 0;
		string impInfo = "";

		for (int q = 0; q < impressions.contract_code_size(); ++q)
		{
			string con_code = impressions.contract_code(q);
			if (con_code != "")
			{
				commrequest.at = BID_PMP;
				impressionobject.dealprice.insert(make_pair(con_code, impressionobject.bidfloor));
			}
		}

		for (int j = 0; j < impressions.creative_specs_size(); ++j)
		{

			advid = impressions.creative_specs(j);
			if (advid != 0)
			{
				data = data + "|" + intToString(advid);
			}
		}

		for (int q = 0; q < impressions.creative_specs_size(); ++q)
		{
			advid = impressions.creative_specs(q);
			impressionobject.ext.advid = advid;
			writeLog(g_logid_local, LOGINFO, "bid=%s, creative_specs=%d ", adrequest.id().c_str(), advid);

			if (advid == 0){
				continue;
			}

			iter = advid_table.find(advid);
			if (iter == advid_table.end()){
				continue;
			}

			impInfo = iter->second;
			get_ImpInfo(impInfo, advtype, w, h, tLen, dLen, proi);
			impressionobject.ext.adv_priority = proi;

			if (advtype == 1)
			{
				impressionobject.type = IMP_BANNER;
				init_com_banner_request_object(impressionobject.banner);
				impressionobject.banner.w = w;
				impressionobject.banner.h = h;
				if (impressions.multimedia_type_white_list_size() > 0)
				{
					string ban_type = impressions.multimedia_type_white_list(0);
					if (ban_type == "jpeg")
					{
						impressionobject.banner.btype.insert(ADTYPE_GIF_IMAGE);
					}
				}
			}
			else if (advtype == 2)
			{
				writeLog(g_logid_local, LOGDEBUG, "id=%s,pid=%lld", commrequest.id.c_str(), impressions.placement_id());
				impressionobject.type = IMP_NATIVE;
				init_com_native_request_object(impressionobject.native);
				set_native(impressionobject.native, impressions, w, h, tLen, dLen, advid);
			}

			commrequest.imp.push_back(impressionobject);
		}
	}

	if (adrequest.has_user())
	{
		const gdt::adx::BidRequest::User &user = adrequest.user();
		if (user.id() != "")
		{
			commrequest.user.id = user.id();
		}
		const gdt::adx::BidRequest::User::Demographic &Demographic = user.user_demographic();
		if (Demographic.gender() == gdt::adx::BidRequest::User::Demographic::GENDER_UNKNOWN)
		{
			commrequest.user.gender = GENDER_UNKNOWN;
		}
		else if (Demographic.gender() == gdt::adx::BidRequest::User::Demographic::GENDER_FEMALE)
		{
			commrequest.user.gender = GENDER_FEMALE;
		}
		else
		{
			commrequest.user.gender = GENDER_MALE;
		}
	}

	if (adrequest.has_app())
	{
		const gdt::adx::BidRequest::App &app = adrequest.app();
		commrequest.app.id = app.app_bundle_id();
		uint32 app_cat = (uint32)app.industry_id();
		vector<uint32_t> &appcat = appcattable[app_cat];
		va_cout("app size= %d", appcat.size());
		for (uint32_t i = 0; i < appcat.size(); ++i)
			commrequest.app.cat.insert(appcat[i]);
	}

	if (commrequest.device.dids.size() == 0 && commrequest.device.dpids.size() == 0)
	{
		errorcode = E_REQUEST_DEVICE_ID;
		writeLog(g_logid_local, LOGERROR, "bid=%s dids and dpids size is zero!", adrequest.id().c_str());
		goto release;
	}

	if (0 == commrequest.imp.size())
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "bid=%s imp size is zero!", adrequest.id().c_str());
		goto release;
	}

	if (commrequest.device.location == 0 ||
		commrequest.device.location > 1156999999 ||
		commrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!",
			adrequest.id().c_str(), commrequest.device.ip.c_str(), commrequest.device.location);
		errorcode = E_REQUEST_DEVICE_IP;
	}

	MY_DEBUG
release:
	commrequest.cur.insert(string("CNY"));

badrequest:
	return errorcode;
}

// convert common response to gdt response
void parse_common_response(IN COM_REQUEST &commrequest, IN COM_RESPONSE &commresponse,
						   IN ADXTEMPLATE &tanxmessage, OUT gdt::adx::BidResponse &bidresponse,
						   OUT string &responseret)
{
#ifdef DEBUG
	cout << "id = " << commresponse.id << endl;
	cout << "gdt = " << commresponse.bidid << endl;
#endif
	// replace result

	for (uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = commresponse.seatbid[i];
		gdt::adx::BidResponse_SeatBid *seatBit = bidresponse.add_seat_bids();

		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT &bidobject = seatbidobject.bid[j];
			gdt::adx::BidResponse_Bid *bidresponseads = seatBit->add_bids();

			if (NULL == bidresponseads)
				continue;

			string impid = bidobject.impid;
			seatBit->set_impression_id(impid);
			writeLog(g_logid_local, LOGINFO, "bid=%s,impid=%s", commresponse.id.c_str(), impid.c_str());

			//string mapid = bidobject.mapid;
			string mapid = bidobject.creative_ext.id;
			bidresponseads->set_creative_id(mapid);
			writeLog(g_logid_local, LOGINFO, "bid=%s,mapid=%s", commresponse.id.c_str(), mapid.c_str());

			double price = bidobject.price * 100;
			writeLog(g_logid_local, LOGINFO, "bid=%s,price=%lf", commresponse.id.c_str(), price);
			bidresponseads->set_bid_price((int32_t)(price));


			char ext[MID_LENGTH] = "";
            sprintf(ext, "%s", get_trace_url_par(commrequest, commresponse).c_str());
			va_cout("imp_cli_url = %s", ext);
			bidresponseads->set_impression_param(ext);
			bidresponseads->set_click_param(ext);

		}
	}
#ifdef DEBUG
	google::protobuf::TextFormat::PrintToString(bidresponse, &responseret);
	print_com_response(commresponse);
	cout << "response data is : " << responseret.c_str() << endl;
#endif

	return;
}

void gdt_adapter(IN uint64_t ctx, IN uint8_t index,
	IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid)
{
	char *senddata = NULL;
	uint32_t sendlength = 0;
	gdt::adx::BidRequest adrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST commrequest;
	COM_RESPONSE commresponse;
	gdt::adx::BidResponse adresponse;
	uint8_t viewtype = 0;
	ADXTEMPLATE adxtemplate;
	struct timespec ts2, ts3, ts4;
	long lasttime2 = 0;
	string str_ferrcode = "";
	string outdate = "";
	string spec_data;
	vector<int> all_imp_index; // 存索引

	getTime(&ts3);

	if (recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
	{// 不可能发生
		err = E_BID_PROGRAM;
		writeLog(g_logid_local, LOGERROR, "recv data is error!");
		goto release;
	}

	parsesuccess = adrequest.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		va_cout("parse data from array failed!");
		err = E_BAD_REQUEST;
		goto release;
	}

	err = parse_common_request(adrequest, commrequest, adresponse, spec_data);
	if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "id = %s the request is not suit for bidding!", adrequest.id().c_str());
		va_cout("the request is not suit for bidding!");
		goto returnresponse;
	}

	all_imp_index = cal_random_index(commrequest.imp);
	flow_bid.set_req(commrequest);

	err = E_UNKNOWN;
	for (int i = 0; i < all_imp_index.size(); i++)
	{
		getTime(&ts2);
		COM_REQUEST commreq_tmp = commrequest;
		commreq_tmp.imp.clear();
		commreq_tmp.imp.push_back(commrequest.imp[all_imp_index[i]]);

		COM_RESPONSE commres_tmp;
		ADXTEMPLATE adxtemplate_tmp;
		
		ferrcode = bid_filter_response(ctx, index, commreq_tmp,
			NULL, filter_group_ext, filter_bid_price, filter_bid_ext,
			&commres_tmp, &adxtemplate_tmp);

		str_ferrcode = bid_detail_record(900, 50000, ferrcode);
		err = ferrcode.errcode;

		getTime(&ts4);
		lasttime2 = (ts4.tv_sec - ts2.tv_sec) * 1000000 + (ts4.tv_nsec - ts2.tv_nsec) / 1000;
		writeLog(g_logid_local, LOGDEBUG, "%s, one creative_specs spent time:%ld, err:0x%x,desc:%s,spec_data:%d",
			adrequest.id().c_str(), lasttime2, err, str_ferrcode.c_str(), commreq_tmp.imp[0].ext.advid);

		if (err == E_SUCCESS || i == all_imp_index.size()-1)
		{
			commrequest = commreq_tmp;
			adxtemplate = adxtemplate_tmp;
			commresponse = commres_tmp;
			flow_bid.set_bid_flow(ferrcode);
			g_ser_log.send_bid_log(index, commrequest, commresponse, err);
			break;
		}
	}

	if (err == E_SUCCESS){
		parse_common_response(commrequest, commresponse, adxtemplate, adresponse, outdate);
	}

returnresponse:
	if (err == E_BID_PROGRAM)
	{
		syslog(LOG_INFO, "redis connect invalid");
		kill(getpid(), SIGTERM);
	}

	sendlength = adresponse.ByteSize();
	senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
	if (senddata == NULL)
	{
		va_cout("allocate memory failure!");
		writeLog(g_logid_local, LOGERROR, "allocate memory failure!");
		goto release;
	}

	adresponse.SerializeToArray(senddata, sendlength);
	data = string("Content-Type: application/octet-stream\r\nContent-Length: ") + 
		intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

	if (err == E_SUCCESS)
	{
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		numcount[index]++;
		if (numcount[index] % 1000 == 0)
		{
			google::protobuf::TextFormat::PrintToString(adresponse, &outdate);
			writeLog(g_logid_local, LOGDEBUG, string("response :") + outdate);
			numcount[index] = 0;
		}
	}
	adresponse.clear_seat_bids();

	getTime(&ts4);
	lasttime2 = (ts4.tv_sec - ts3.tv_sec) * 1000000 + (ts4.tv_nsec - ts3.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,spec_data:%s",
		VERSION_BID, adrequest.id().c_str(), lasttime2, err, spec_data.c_str());
	va_cout("%s,spent time:%ld,err:0x%x", adrequest.id().c_str(), lasttime2, err);

release:
	flow_bid.set_err(err);
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
	return;
}
