#include <map>
#include <vector>
#include <string>
#include <errno.h>
#include <algorithm>
#include <set>
#include <google/protobuf/text_format.h>
#include "iqiyi_adapter.h"
#include "../../common/bid_filter.h"
#include "bid_response.pb.h"
#include "bid_request.pb.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
using namespace std;


#define  TRACKEVENT 5
extern uint64_t g_logid_local, g_logid, g_logid_response;
extern map<uint32_t, vector<uint32_t> > app_cat_table;
extern map<uint32_t, vector<uint32_t> > adv_cat_table_adxi;
extern map<uint64_t, vector<uint32_t> > adv_cat_table_price;
string trackevent[5] = { "start", "firstQuartile", "midpoint", "thirdQuartile", "complete" };
const char *ftypeA[] = { "image/png", "image/jpeg", "image/gif", "video/x-flv", "video/mp4" };
extern MD5_CTX hash_ctx;
extern uint64_t geodb;
extern int *numcount;
#define ICON_URL "http://pic7.qiyipic.com/common/20160825/75a22168621641afbae00a28289af2dd.png"


// convert tanx request to common request
int parse_common_request(IN ads_serving::proto::BidRequest &bidrequest, OUT COM_REQUEST &commrequest, 
	OUT ads_serving::proto::BidResponse &bidresponse)
{
	int errorcode = E_SUCCESS;
	string userID;
	int paltformID = 0;
	int devicetype = 0;
	int os = OS_UNKNOWN;
	string osv = "";
	char* model_str = NULL;

	commrequest.cur.insert(string("CNY"));
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

	if (bidrequest.has_site())
	{
		if (bidrequest.site().has_content())
		{
			const ::ads_serving::proto::Content &content = bidrequest.site().content();
			if (content.has_channel_id())
			{
				int64_t channel_id = content.channel_id();
				vector<uint32_t> &v_cat = app_cat_table[channel_id];
				for (uint32_t i = 0; i < v_cat.size(); i++)
					commrequest.app.cat.insert(v_cat[i]);
			}
		}
	}

	if (bidrequest.has_device())  // app bundle
	{
		const ::ads_serving::proto::Device &dev = bidrequest.device();
		if (dev.has_ua())
			commrequest.device.ua = dev.ua();

		if (dev.has_ip())
		{
			if (dev.ip() != "")
			{
				commrequest.device.ip = dev.ip();
				commrequest.device.location = getRegionCode(geodb, commrequest.device.ip);
			}
		}

		if (dev.has_geo())
		{
			const ads_serving::proto::Geo &geo = dev.geo();
			{
				writeLog(g_logid_local, LOGDEBUG, "contry: %d, metro:%d, city: %d", geo.country(), geo.metro(), geo.city());
			}
			//commrequest.device.geo.lat = ;
			//commrequest.device.geo.lon = ;
		}

		if (dev.has_connection_type())
		{

			int conectiontype = dev.connection_type();
			switch (conectiontype)
			{
			case 1://Ethernet
				conectiontype = CONNECTION_CELLULAR_UNKNOWN;
				break;
			case 2:
				conectiontype = CONNECTION_WIFI;
				break;
			}
			commrequest.device.connectiontype = conectiontype;
		}

		if (dev.has_platform_id())//device dpids or dpid
		{
			paltformID = dev.platform_id();
			switch (paltformID)
			{
				/*case 11:
				case 12:
				{
				devicetype = DEVICE_TV;
				break;
				}*/

			case 21://IPad H5（贴片可点击）
			case 22:
			{
				devicetype = DEVICE_TABLET;
				//os = OS_IOS;
				break;
			}
			case 23:
			{
				devicetype = DEVICE_TABLET;
				//os = OS_ANDROID;
				break;
			}
			case 25:
			{
				devicetype = DEVICE_TABLET;
				//os = OS_WINDOWS;
				break;
			}
			case 32:
				//case 312:iPhoneH5（贴片不可点击）
			{
				devicetype = DEVICE_PHONE;
				//os = OS_IOS;
				break;
			}
			case 33:
			case 311://Android手机H5
			{
				devicetype = DEVICE_PHONE;
				//os = OS_ANDROID;
				break;
			}
			case 35:
			{
				devicetype = DEVICE_PHONE;
				//os = OS_WINDOWS;
				break;
			}
			default:
			{
				devicetype = DEVICE_UNKNOWN;
				break;
			}
			}
			commrequest.device.devicetype = devicetype;
		}
		if (dev.has_os())
		{
			string os_str = dev.os();
			if (os_str == "android")
			{
				os = OS_ANDROID;
				commrequest.is_secure = 0;
			}
			else if (os_str == "ios")
			{
				os = OS_IOS;
				commrequest.is_secure = 1;
			}
			else if (os_str == "windows")
			{
				os = OS_WINDOWS;
			}
		}
		if (dev.has_android_id())
		{
			string dpid = dev.android_id();
			if (dpid != "" && os == OS_ANDROID)
			{
				commrequest.device.dpids.insert(make_pair(DPID_ANDROIDID, dpid));
			}
		}
		if (dev.has_os_version())
		{
			osv = dev.os_version();
		}
		if (dev.has_model())
		{
			char* model_str = const_cast<char *>(dev.model().c_str());
			int decode_len = urldecode(model_str, strlen(model_str));
			commrequest.device.model = model_str;
		}
	}
	else    // set device null
	{
		errorcode = E_REQUEST_DEVICE;
		writeLog(g_logid_local, LOGERROR, "id = %s have no device", bidrequest.id().c_str());
	}

	commrequest.device.os = os;
	commrequest.device.osv = osv;
	commrequest.device.make = DEFAULT_MAKE;
	commrequest.device.makestr = "unknown";

	if (bidrequest.has_user())
	{
		if (bidrequest.user().has_id())
			userID = bidrequest.user().id();

		// The unique identifier of this user on the exchange.
		// For IOS, this is IDFA or UDID. For Android, this is IMEI or MAC address.
		if (userID != "")
		{
			switch (os)
			{
			case OS_ANDROID:
			{
				switch (userID.size())
				{
				case LEN_IMEI_SHORT:
				case LEN_IMEI_LONG:
				{
					commrequest.device.dids.insert(make_pair(DID_IMEI, userID));
					break;
				}
				case LEN_MAC_SHORT:
				case LEN_MAC_LONG:
				{
					commrequest.device.dids.insert(make_pair(DID_MAC, userID));
					break;
				}
				case LEN_MD5:
				{
					commrequest.device.dids.insert(make_pair(DID_IMEI_MD5, userID));
					break;
				}
				default:
				{
					commrequest.device.dids.insert(make_pair(DID_OTHER, userID));
					break;
				}
				}
				break;
			}
			case OS_IOS:
			{
				switch (userID.size())
				{
				case LEN_IDFA:
				{
					commrequest.device.dpids.insert(make_pair(DPID_IDFA, userID));
					break;
				}
				default:
				{
					commrequest.device.dpids.insert(make_pair(DPID_OTHER, userID));
					break;
				}
				}
				break;
			}
			default:
			{
				commrequest.device.dpids.insert(make_pair(DID_OTHER, userID));
				break;
			}
			}
		}
	}

	/*if (bidrequest.has_category())
		{
		vector<uint32_t> &appcat = appcattable[bidrequest.category()];
		for (uint32_t i = 0; i < appcat.size(); ++i)
		commrequest.app.cat.insert(appcat[i]);
		}    */
	// tanx new support one adzone
	for (int i = 0; i < bidrequest.imp_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impObj;
		init_com_impression_object(&impObj);
		impObj.bidfloorcur = "CNY";
		impObj.requestidlife = 3600;
		const ads_serving::proto::Impression &imp = bidrequest.imp(i);
		if (imp.has_id())
			impObj.id = bidrequest.imp(i).id();
		else
		{
			errorcode = E_REQUEST_IMP_NO_ID;
			//goto badrequest;
		}
		// The advertisements with these tags will be blocked on this impression.
		// Ad tag can be understood as the product type in an ad.
		for (int j = 0; j < imp.blocked_ad_attribute_size(); j++)
		{

		}

		for (int j = 0; j < imp.blocked_ad_tag_size(); j++)
		{
			vector<uint32_t> &v_cat = adv_cat_table_adxi[imp.blocked_ad_tag(j)];
			for (uint32_t i = 0; i < v_cat.size(); i++)
				commrequest.bcat.insert(v_cat[i]);
		}
		if (imp.is_pmp())
		{
			commrequest.at = BID_PMP;
		}

		if (imp.has_banner())
		{
			impObj.type = IMP_BANNER;
			const ::ads_serving::proto::Banner &banner = imp.banner();
			//uint64_t ad_zone_id = 0;
			if (banner.has_ad_zone_id())
			{
				switch (banner.ad_zone_id())
				{
				default:
					errorcode = E_REQUEST_IMP_TYPE;
					break;
				}
			}

		}
		else if (imp.has_video())
		{
			impObj.type = IMP_VIDEO;
			const ads_serving::proto::Video &video = imp.video();
			if (video.has_ad_zone_id())
			{
				switch (video.ad_zone_id())
				{
				case 1000000000381:
				{//iPhoneH5(贴片不可点击)
					impObj.video.is_limit_size = false;
					if (paltformID == 312)
						errorcode = E_REQUEST_IMP_TYPE;
					break;
				}
				case 1000000000456://移动端通用角标
				{
					impObj.type = IMP_BANNER;
					switch (paltformID)
					{
					case 22:
					case 23:
					{
						impObj.banner.w = 240;
						impObj.banner.h = 200;
						break;
					}
					case 32:
					case 33:
					{
						impObj.banner.w = 180;
						impObj.banner.h = 150;
						break;
					}
					default:
					{
						errorcode = E_REQUEST_IMP_TYPE;
						break;
					}
					}
					break;
				}
				case 1000000000041:
				case 1000000000410://移动端TV端通用暂停广告
				{
					impObj.type = IMP_BANNER;
					switch (paltformID)
					{
					case 22:
					{
						impObj.banner.w = 950;
						impObj.banner.h = 790;
						break;
					}
					/*case 23:
					{
					break;
					}*/

					case 32:
					case 33:
					{
						impObj.banner.w = 600;
						impObj.banner.h = 500;
						break;
					}
					default:
					{
						errorcode = E_REQUEST_IMP_TYPE;
						break;
					}
					}
					break;
				}
				default:
					errorcode = E_REQUEST_IMP_TYPE;
				}
			}

			if (video.has_ad_type())
			{
				int ad_type = video.ad_type();
				switch (ad_type)
				{
				case 1:
				{
					ad_type = DISPLAY_FRONT_PASTER;
					break;
				}
				case 2:
				{
					ad_type = DISPLAY_MIDDLE_PASTER;
					break;
				}
				case 3:
				{
					ad_type = DISPLAY_BACK_PASTER;
					break;
				}
				case 4:
				{
					ad_type = DISPLAY_CORNER;
					break;
				}
				case 6:
				{
					ad_type = DISPLAY_PAUSE;
					break;
				}
				case 9:
				{
					ad_type = DISPLAY_SUPERNATANT;
					break;
				}
				default:
				{
					errorcode = E_REQUEST_VIDEO_ADTYPE_INVALID;
				}
				}
				impObj.video.display = ad_type;
			}
			if (video.has_maxduration())
				impObj.video.maxduration = video.maxduration();
			if (video.has_minduration())
				impObj.video.minduration = video.minduration();
			if (video.has_w())
				impObj.video.w = video.w();
			if (video.has_h())
				impObj.video.h = video.h();
			//impObj.video.mimes.insert(VIDEOFORMAT_MP4);
			if (video.has_startdelay())
			{

			}
			// Indicates whether the ad impression is linear or non-linear.
			// 1. Linear, example: pre-roll, mid-roll and post-roll.
			// 2. Non-linear, example: overlay, video link, pause, and tool bar.
			if (video.has_linearity())
			{
				// This field is meaningful only when this impression is linear. It indicates
				// whether "maxduration" is equal to the total duration this impression holds.
				// That's to say, the entire (pre/mid/post)-roll is available if it is true.
				if (video.is_entire_roll())
				{
					//调整视频时长
				}
			}
		}


		if (imp.has_bidfloor())
			impObj.bidfloor = (double)(imp.bidfloor() / 100.0);

		for (int j = 0; j < imp.floor_price_size(); ++j)
		{
			uint64_t industry = (uint32_t)imp.floor_price(j).industry();
			double price = (double)(imp.floor_price(j).price() / 100.0);
			vector<uint32_t> &v_cat = adv_cat_table_price[industry];
			for (int k = 0; k < v_cat.size(); ++k)
			{
				impObj.fprice.insert(pair<int, double>(v_cat[k], price));
			}
		}

		commrequest.imp.push_back(impObj);
	}


	if (0 == commrequest.imp.size())  // 没有符合的曝光对象
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "id = %s imp size is zero!", bidrequest.id().c_str());
	}

badrequest:
	return errorcode;
}

void callback_insert_pair_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<uint32_t> > &table = *((map<uint32_t, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[atoi(key_start.c_str())];
	s_val.push_back(com_cat);
}

void callback_insert_pair_int64(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint64_t, vector<uint32_t> > &table = *((map<uint64_t, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[atoll(key_start.c_str())];
	s_val.push_back(com_cat);
}

static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	double bidfloor = 0;
	bidfloor = crequest.imp[0].bidfloor;

	if ((price - bidfloor - 0.01) >= 0.000001)
		return true;

	cflog(g_logid_local, LOGDEBUG, "%s,bidfloor:%f,crprice:%f", crequest.id.c_str(), bidfloor, price);
	return false;
}

static bool filter_group_ext(IN const COM_REQUEST &crequest, IN const GROUPINFO_EXT &ext)
{
	if (ext.advid != "")
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

// convert common response to tanx response
void parse_common_response(IN COM_REQUEST &commrequest, IN COM_RESPONSE &commresponse, 
	IN ADXTEMPLATE &adxtemp, OUT ads_serving::proto::BidResponse &bidresponse)
{
	string trace_url_par = string("&") + get_trace_url_par(commrequest, commresponse);
// replace result
    for (uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
    {
        COM_SEATBIDOBJECT &seatbidobject = commresponse.seatbid[i];
        for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			string duration = "";
			COM_BIDOBJECT &cbidobject = seatbidobject.bid[j];
			ads_serving::proto::Seatbid* seabid = bidresponse.add_seatbid();
			if (seabid == NULL)
				return;

			ads_serving::proto::Bid  *bidObj = seabid->add_bid();
			if (bidObj == NULL)
				return;

			//replace_https_url(adxtemp,commrequest.is_secure);

			bidObj->set_id(cbidobject.mapid);
			bidObj->set_impid(cbidobject.impid);

			double price = cbidobject.price * 100;
			bidObj->set_price((int)price);
			//adm和crid字段需讨论

			if (cbidobject.creative_ext.id.size() > 0)
				bidObj->set_crid(cbidobject.creative_ext.id);

			if (cbidobject.creative_ext.ext.size() > 0)
			{
				string ext = cbidobject.creative_ext.ext;
				json_t *tmproot = NULL;
				json_t *label;
				json_parse_document(&tmproot, ext.c_str());
				if (tmproot != NULL)
				{
					if ((label = json_find_first_label(tmproot, "duration")) != NULL)
						duration = label->child->text;
					json_free_value(&tmproot);
				}
			}

			if (commrequest.imp[0].video.display == DISPLAY_CORNER)
			{
				duration = "00:02:00";
			}
			/*	if(cbidobject.sourceurl.size() > 0)
					bidObj->set_adm(cbidobject.sourceurl);*/

			if (!adxtemp.adms.empty() && adxtemp.adms.size() > 0)
			{
				string adm = "";
				//根据os和type查找对应的adm

				for (int count = 0; count < adxtemp.adms.size(); count++)
				{
					if (adxtemp.adms[count].os == commrequest.device.os && adxtemp.adms[count].type == cbidobject.type)
					{
						adm = adxtemp.adms[count].adm;
						break;
					}
				}

				if (adm.size() > 0)
				{
					/*char desc[128] ="";
					int index = 0;
					sprintf(desc, "program='%s' width='%d' height='%d' xPosition='%s' yPosition='%s'", "iqiyi", cbidobject.w, cbidobject.h, "left", "bottom");
					replaceMacro(adm, "#DESC#", desc);
					switch(cbidobject.ftype)
					{
					case 17:
					{
					index = 0;
					break;
					}
					case 18:
					{
					index = 1;
					break;
					}
					case 19:
					{
					index = 2;
					break;
					}
					case 33:
					{
					index = 3;
					break;
					}
					case 34:
					{
					index = 4;
					break;
					}
					default:
					break;
					}
					replaceMacro(adm, "#FTYPE#", ftypeA[index]);*/

					//	replaceMacro(adm, "#ADX#", intToString(ADX_IQIYI).c_str());
					replaceMacro(adm, "#CRID#", cbidobject.creative_ext.id.c_str());
					if (cbidobject.groupinfo_ext.advid.size() > 0)
						replaceMacro(adm, "#ADID#", cbidobject.groupinfo_ext.advid.c_str());
					replaceMacro(adm, "#DURATION#", duration.c_str());
					//replaceMacro(adm, "#DURATION#", "");
					/*if(cbidobject.sourceurl.size() > 0)
						replaceMacro(adm, "#SOURCEURL#", cbidobject.sourceurl.c_str());*/

					//IURL需要替换
					string aurl;
					string iurl;
					string cturl;
					string curl;

					if (adxtemp.aurl.size() > 0)
					{
						if (cbidobject.ftype == ADFTYPE_VIDEO_MP4)
						{
							for (int k = 0; k < TRACKEVENT; k++)
							{
								aurl += "<Tracking event=\"" + trackevent[k] + "\"><![CDATA[" + adxtemp.aurl + 
									"&event=" + trackevent[k] + "]]></Tracking>";
							}
						}
					}
					replaceMacro(adm, "#AURL#", aurl.c_str());

					if (adxtemp.iurl.size() > 0 || cbidobject.imonitorurl.size() > 0)
					{
						int k, j = 0;
						for (k = 0; k < adxtemp.iurl.size(); ++k, ++j)
						{
							string temp_iurl = adxtemp.iurl[k] + trace_url_par;
							iurl += "<Impression id=\"" + intToString(j) + "\"><![CDATA[" + temp_iurl + "]]></Impression>";
						}

						for (k = 0; k < cbidobject.imonitorurl.size(); ++k, ++j)
						{
							iurl += "<Impression id=\"" + intToString(j) + "\"><![CDATA[" + cbidobject.imonitorurl[k] + "]]></Impression>";
						}
						//iurl=iurl.substr(0, iurl.length() - 1);
					}
					replaceMacro(adm, "#IURL#", iurl.c_str());

					int ctype = 0;
					string curl_target = cbidobject.curl;
					if (curl_target != "")
					{
						switch (cbidobject.ctype)
						{
						case  CTYPE_DOWNLOAD_APP_FROM_APPSTORE:
						{
							ctype = 4;
							break;
						}
						case  CTYPE_DOWNLOAD_APP:
						{
							ctype = 4;
							curl = "<ClickThrough type=\"" + intToString(ctype) + "\"><![CDATA[" + curl_target + "]]></ClickThrough>";
							break;
						}
						default:
						{
							ctype = 0;
							curl = "<ClickThrough type=\"" + intToString(ctype) + "\"><![CDATA[" + curl_target + "]]></ClickThrough>";
						}
						}
					}
					replaceMacro(adm, "#CURL#", curl.c_str());

					if (adxtemp.cturl.size() > 0 || cbidobject.cmonitorurl.size() > 0)
					{
						int k = 0;
						for (; k < adxtemp.cturl.size(); k++)
						{
							string temp_cturl = adxtemp.cturl[k] + trace_url_par;
							cturl += "<ClickTracking><![CDATA[" + temp_cturl + "&curl=]]></ClickTracking>";
						}

						for (k = 0; k < cbidobject.cmonitorurl.size(); k++)
						{
							cturl += "<ClickTracking><![CDATA[" + cbidobject.cmonitorurl[k] + "]]></ClickTracking>";
						}

						if (cbidobject.ctype == CTYPE_DOWNLOAD_APP_FROM_APPSTORE)
						{
							cturl += "<ClickTracking type=\"" + intToString(ctype) + "\"><![CDATA[" + curl_target + "]]></ClickTracking>";
						}
					}
					replaceMacro(adm, "#CTURL#", cturl.c_str());
					string icon = "<Icon><StaticResource><![CDATA[" + string(ICON_URL) + "]]></StaticResource></Icon>";
					replaceMacro(adm, "#ICON#", icon.c_str());
					replaceMacro(adm, "'", "\"");

					//cout <<"adm :"<<adm <<endl;
					//adm = adm.substr(0, adm.length() - 1);
					bidObj->set_adm(adm);
				}
			}
		}
	}

	return;
}

int iqiyi_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata,
	OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	char *data = NULL;
	FULL_ERRCODE ferrcode = { 0 };
	//string strData;
	uint32_t length = 0;
	ads_serving::proto::BidRequest bidrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;

	COM_REQUEST commrequest;
	COM_RESPONSE commresponse;
	ads_serving::proto::BidResponse bidresponse;
	ADXTEMPLATE adxtemplate;
	struct timespec ts1, ts2;
	long lasttime = 0;
	string str_ferrcode = "";

	getTime(&ts1);

	if (recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
	{
		err = E_BID_PROGRAM;
		writeLog(g_logid_local, LOGERROR, "recv data is error!");
		goto exit;
	}

	parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		va_cout("parse data from array failed!");
		err = E_BAD_REQUEST;
		goto exit;
	}

	err = parse_common_request(bidrequest, commrequest, bidresponse);
	if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);
		goto exit;
	}
	flow_bid.set_req(commrequest);

	if (commrequest.device.location == 0 || commrequest.device.location > 1156999999 || commrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s, getRegionCode ip:%s location:%d failed!", 
			commrequest.id.c_str(), commrequest.device.ip.c_str(), commrequest.device.location);
		err = E_REQUEST_DEVICE_IP;
	}
	else if (err == E_SUCCESS)
	{
		uint64_t cm;
		char strcm[16] = "";
		cm = ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000;
		snprintf(strcm, 16, "%llu", cm);
		commrequest.id = string(strcm) + commrequest.id;

		init_com_message_response(&commresponse);
		ferrcode = bid_filter_response(ctx, index, commrequest, 
			NULL, filter_group_ext, filter_bid_price, filter_bid_ext, 
			&commresponse, &adxtemplate);
		err = ferrcode.errcode;
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		flow_bid.set_bid_flow(ferrcode);

		g_ser_log.send_bid_log(index, commrequest, commresponse, err);
	}

	if (err == E_SUCCESS)
	{
		string responseret;
		parse_common_response(commrequest, commresponse, adxtemplate, bidresponse);
		va_cout("parse common response to iqiyi response success!");

		length = bidresponse.ByteSize();
		data = (char *)calloc(1, sizeof(char) * (length + 1));
		if (data == NULL)
		{
			va_cout("allocate memory failure!");
			writeLog(g_logid_local, LOGERROR, "allocate memory failure!");
			senddata = "Status: 204 OK\r\n\r\n";
			goto exit;
		}
		numcount[index]++;
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		if (numcount[index] % 1000 == 0)
		{
			google::protobuf::TextFormat::PrintToString(bidresponse, &responseret);
			writeLog(g_logid_local, LOGDEBUG, "response id:" + commrequest.id + ", " + responseret);
			numcount[index] = 0;
		}
		bidresponse.SerializeToArray(data, length);
		//bidresponse.SerializeToString(&strData);
		bidresponse.Clear();
		senddata = "Content-Type: application/x-protobuf\r\nContent-Length: " + intToString(length) + "\r\n\r\n" + string(data, length);
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", 
			commresponse.bidid.c_str(), commresponse.seatbid[0].bid[0].mapid.c_str(), 
			commrequest.device.deviceid.c_str(), commrequest.device.deviceidtype, 
			commresponse.seatbid[0].bid[0].price);
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, commrequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%d, err:0x%x", bidrequest.id().c_str(), lasttime, err);
	if (err != E_REDIS_CONNECT_INVALID)
		err = E_SUCCESS;

exit:
	if (data != NULL)
	{
		free(data);
		data = NULL;
	}
	flow_bid.set_err(err);
	return err;
}
