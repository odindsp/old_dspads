/*
 * goyoo_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <algorithm>
#include <google/protobuf/text_format.h>
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "goyoo_response.h"


extern map<int, vector<uint32_t> > app_cat_table;
extern map<string, uint16_t> dev_make_table;
extern int *numcount;
extern uint64_t geodb;
extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local, g_logid_response;
extern MD5_CTX hash_ctx;


void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int, vector<uint32_t> > &table = *((map<int, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[atoi(key_start.c_str())];

	s_val.push_back(com_cat);
}

void callback_insert_pair_string_r(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<uint32_t> > &table = *((map<uint32_t, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(key_start.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[com_cat];

	s_val.push_back(atoi(val.c_str()));
}

int parseGoyooRequest(BidRequest &bidrequest, COM_REQUEST &crequest, BidResponse &bidresponse)
{
	int errorcode = E_SUCCESS;
	if (bidrequest.has_id())
	{
		crequest.id = bidrequest.id();
		bidresponse.set_id(crequest.id);
		bidresponse.set_bidid(crequest.id);
	}
	else
	{
		errorcode = E_BAD_REQUEST;
		goto exit;
	}

	for (int i = 0; i < bidrequest.imp_size(); ++i)
	{
		COM_IMPRESSIONOBJECT cimpObj;
		Imp imp = bidrequest.imp(i);
		init_com_impression_object(&cimpObj);
		cimpObj.bidfloorcur = "CNY";

		if (imp.has_id())

			cimpObj.id = imp.id();
		else
		{
			errorcode = E_BAD_REQUEST;
			goto exit;
		}

		if (imp.has_banner())
		{
			cimpObj.type = IMP_BANNER;
			//设定request id的生存期为120分钟
			cimpObj.requestidlife = 7200;
			Banner banner = imp.banner();
			if (banner.has_w())
				cimpObj.banner.w = banner.w();
			if (banner.has_h())
				cimpObj.banner.h = banner.h();
			for (int j = 0; j < banner.btype_size(); ++j)
			{
				//需要转换
			}

			for (int j = 0; j < banner.battr_size(); ++j)
			{
				//需要转换
				string attr = CreativeAttribute_Name(banner.battr(j));
			}

			int adpos = banner.pos();
			switch (adpos)
			{
			case 1:
			case 4:
				cimpObj.adpos = ADPOS_HEADER;
				break;
			case 3:
			case 5:
				cimpObj.adpos = ADPOS_FOOTER;
				break;
			case 6:
				cimpObj.adpos = ADPOS_SIDEBAR;
				break;
			case 7:
				cimpObj.adpos = ADPOS_FULL;
				break;
			default:
				cimpObj.adpos = ADPOS_UNKNOWN;
				break;
			}
		}

		if (imp.has_bidfloor())
		{
			cimpObj.bidfloor = imp.bidfloor() / 100;
		}
		crequest.imp.push_back(cimpObj);
	}


	if (bidrequest.has_site())
	{

	}

	if (bidrequest.has_app())
	{
		App app = bidrequest.app();
		if (app.has_id())
			crequest.app.id = app.id();
		if (app.has_name())
			crequest.app.name = app.name();
		if (app.has_bundle())
			crequest.app.bundle = app.bundle();
		if (app.has_storeurl())
			crequest.app.storeurl = app.storeurl();
		if (app.has_cat())
		{
			//cat需转换
			vector<uint32_t> &v_cat = app_cat_table[app.cat()];
			for (uint32_t i = 0; i < v_cat.size(); ++i)
				crequest.app.cat.insert(v_cat[i]);
		}
		if (app.has_sectioncat())
		{

		}
		if (app.has_pagecat())
		{

		}
	}

	if (bidrequest.has_user())
	{
		crequest.user.id = bidrequest.user().id();
		if (bidrequest.user().has_yob())
			crequest.user.yob = bidrequest.user().yob();

		string gender = bidrequest.user().gender();
		if (gender == "M")
			crequest.user.gender = GENDER_MALE;
		else if (gender == "F")
			crequest.user.gender = GENDER_FEMALE;
		else
			crequest.user.gender = GENDER_UNKNOWN;
		if (bidrequest.user().has_geo())
		{
			crequest.user.geo.lat = bidrequest.user().geo().lat();
			crequest.user.geo.lon = bidrequest.user().geo().lon();
		}

		if (bidrequest.user().keywords_size() > 0)
		{
			crequest.user.keywords = bidrequest.user().keywords(0);
			for (uint32_t k = 1; k < bidrequest.user().keywords_size(); ++k)
			{
				crequest.user.keywords += ",";
				crequest.user.keywords += bidrequest.user().keywords(k);
			}
		}
	}

	if (bidrequest.has_device())
	{
		Device device = bidrequest.device();
		if (device.has_ua())
			crequest.device.ua = device.ua();
		if (device.has_geo())
		{
			Geo geo = device.geo();
			if (geo.has_lat())
				crequest.device.geo.lat = geo.lat();
			if (geo.has_lon())
				crequest.device.geo.lon = geo.lon();
		}
		if (device.has_ip())
		{
			crequest.device.ip = device.ip();
			if (crequest.device.ip.size() > 0)
			{
				int location = getRegionCode(geodb, crequest.device.ip);
				crequest.device.location = location;
			}
		}

		if (device.has_make())
		{
			//make需转换
			string s_make = device.make();
			//transform(s_make.begin(), s_make.end(), s_make.begin(), ::tolower);
			if (s_make != "")
			{
				map<string, uint16_t>::iterator it;

				crequest.device.makestr = s_make;
				for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
				{
					if (s_make.find(it->first) != string::npos)
					{
						crequest.device.make = it->second;
						break;
					}
				}
			}
		}
		if (device.has_model())
			crequest.device.model = device.model();
		if (device.has_os())
		{
			string os = device.os();
			writeLog(g_logid_local, LOGINFO, "id: %s os is : %s", crequest.id.c_str(), os.c_str());
			string dpid = "";
			string sha1dpid = "";
			string md5dpid = "";
			if (device.has_idfa())
				dpid = device.idfa();
			else if (device.has_androidid())
				dpid = device.androidid();

			if (dpid.size() > 0)
			{
				writeLog(g_logid_local, LOGINFO, "id: %s dpid is : %s", crequest.id.c_str(), dpid.c_str());
				transform(dpid.begin(), dpid.end(), dpid.begin(), ::tolower);
				md5dpid = dpid.substr(0, dpid.find('-'));
				sha1dpid = dpid.substr(dpid.find('-') + 1);
			}

			if (strcasecmp(os.c_str(), "iPhone OS") == 0 || strcasecmp(os.c_str(), "iOS") == 0)
			{
				crequest.device.os = OS_IOS;
				// crequest.device.make = APPLE_MAKE;
				if (check_string_type(dpid, LEN_IDFA, true, false, INT_RADIX_16))//必须是字母数字
				{
					crequest.device.dpids.insert(make_pair(DPID_IDFA, dpid));
				}
				else
				{
					if (md5dpid.size() > 0)
						crequest.device.dpids.insert(make_pair(DPID_IDFA_MD5, md5dpid));
					if (sha1dpid.size() > 0)
						crequest.device.dpids.insert(make_pair(DPID_IDFA_SHA1, sha1dpid));
				}

			}
			else if (strcasecmp(os.c_str(), "Android") == 0)
			{
				crequest.device.os = OS_ANDROID;
				if (check_string_type(dpid, LEN_ANDROIDID, false, false, INT_RADIX_16))//16位且都是数字
				{
					crequest.device.dpids.insert(make_pair(DPID_ANDROIDID, dpid));
				}
				else
				{
					if (md5dpid.size() > 0)
						crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, md5dpid));
					if (sha1dpid.size() > 0)
						crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, sha1dpid));
				}
			}
			else if (strcasecmp(os.c_str(), "Windows") == 0)
			{
				crequest.device.os = OS_WINDOWS;
			}
		}

		if (device.has_osv())
		{
			crequest.device.osv = device.osv();
		}

		if (device.has_imei())
		{
			writeLog(g_logid_local, LOGINFO, "id: %s imei is : %s", crequest.id.c_str(), device.imei().c_str());
			if (device.imei() != "")
			{
				crequest.device.dids.insert(make_pair(DID_IMEI, device.imei()));
			}
		}

		if (device.has_mac())
		{
			writeLog(g_logid_local, LOGINFO, "id: %s mac is : %s", crequest.id.c_str(), device.mac().c_str());
			if (device.mac() != ""  && device.mac() != "null")
				crequest.device.dids.insert(make_pair(DID_MAC, device.mac()));
		}

		if (device.has_carrier())
		{
			const char *carrier = device.carrier().c_str();

			if (strcmp(carrier, "70120") == 0)
				crequest.device.carrier = CARRIER_CHINAMOBILE;
			else if (strcmp(carrier, "70123") == 0)
				crequest.device.carrier = CARRIER_CHINAUNICOM;
			else if (strcmp(carrier, "70121") == 0)
				crequest.device.carrier = CARRIER_CHINATELECOM;
		}
		if (device.has_devicetype())
		{
			int devicetype = device.devicetype();
			if (devicetype == MOBILE)
				crequest.device.devicetype = DEVICE_PHONE;
			else//PC不支持
			{
				errorcode = E_REQUEST_DEVICE_TYPE;
				goto exit;
			}
		}
		if (device.connectiontype())
		{
			int connectiontype = device.connectiontype();
			switch (connectiontype)
			{
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			{
				--connectiontype;
				break;
			}
			default:
			{
				connectiontype = CONNECTION_UNKNOWN;
				break;
			}
			}
			crequest.device.connectiontype = connectiontype;
		}

	}

	/* if(bidrequest.has_test())
	 {

	 }

	 if(bidrequest.has_tmax())
	 {

	 }

	 if(bidrequest.has_at())
	 {

	 }

	 for(int i = 0; i < bidrequest.wseat_size(); ++i)
	 {

	 }
	 */
	if (bidrequest.has_scenario())
	{
		Scenario scenario = bidrequest.scenario();
		if (scenario.has_type())
		{
			writeLog(g_logid_local, LOGINFO, "%s and scenario type is %d", crequest.id.c_str(), scenario.type());
		}
		if (scenario.has_info())
		{
			writeLog(g_logid_local, LOGINFO, crequest.id + " and scenario info is " + scenario.info());
		}
	}
exit:
	return errorcode;
}

static void setGoyooJsonResponse(IN COM_REQUEST &crequest, IN COM_RESPONSE &cresponse, 
	IN ADXTEMPLATE &adxtemp, OUT BidResponse &bidresponse, int errcode)
{
	
	string trace_url_par = string("&") + get_trace_url_par(crequest, cresponse);
	if(errcode == E_SUCCESS)
	{
		for (uint32_t i = 0; i < cresponse.seatbid.size(); ++i)
		{
			SeatBid *seatbid = bidresponse.add_seatbid();
			seatbid->set_seat(SEAT_GOYOO);
			COM_SEATBIDOBJECT &seatbidobject = cresponse.seatbid[i];
			for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
			{
				COM_BIDOBJECT &cbidobject = seatbidobject.bid[j];
				Bid *bid = seatbid->add_bid();
				//用mapid暂时代替DSPID
				bid->set_id(cbidobject.mapid);
				bid->set_impid(cbidobject.impid);
				bid->set_price(cbidobject.price * 100);
				if (cbidobject.adid.size() >0)
				{
					bid->set_adid(cbidobject.adid);
				}
				//替换宏。将结算宏放置在iurl中
				/*if (!adxtemp.nurl.empty() && adxtemp.nurl.size() > 0)
				{
				string nurl = adxtemp.nurl;
				bid->set_nurl(nurl);
				}*/
				/*if (!adxtemp.adms.empty() && adxtemp.adms.size() > 0)
				{
				string adm = "";
				//根据os和type查找对应的adm
				for(int count = 0; count < adxtemp.adms.size(); ++count)
				{
				if(adxtemp.adms[count].os == crequest.device.os && adxtemp.adms[count].type == cbidobject.type)
				{
				adm = adxtemp.adms[count].adm;
				break;
				}
				}

				if(adm.size() > 0)
				{
				replaceMacro(adm, "#SOURCEURL#", cbidobject.sourceurl.c_str());
				bid->set_adm(adm);
				}

				}*/
				//广告家建议使用json
				if (cbidobject.type == ADTYPE_IMAGE)
				{
					char *adm = NULL;
					json_t *root, *label;
					root = label = NULL;

					root = json_new_object();
					jsonInsertString(root, "src", cbidobject.sourceurl.c_str());
					jsonInsertInt(root, "width", cbidobject.w);
					jsonInsertInt(root, "hight", cbidobject.h);
					json_tree_to_string(root, &adm);
					bid->set_adm(adm);
					bid->set_admtype(JSON);
					bid->set_type(IMAGE);
					free(adm);
				}

				if (cbidobject.ctype == 2 && cbidobject.bundle.size() > 0)
					bid->set_bundle(cbidobject.bundle);

				if (adxtemp.iurl.size() > 0)
				{
					string iurl = adxtemp.iurl[0] + trace_url_par;
					bid->set_iurl(iurl);
				}

				for (int i = 0; i < cbidobject.imonitorurl.size(); ++i)
				{

					string imonitorurl = cbidobject.imonitorurl[i];

					bid->add_extiurl(imonitorurl);
				}

				if (cbidobject.cid.size() > 0)
					bid->set_cid(cbidobject.cid);
				if (cbidobject.crid.size() > 0)
					bid->set_crid(cbidobject.crid);

				//bid->set_cat();
				//bid->set_attr();
				bid->set_w(cbidobject.w);
				bid->set_h(cbidobject.h);

				string curl = cbidobject.curl;

				int k = 0;
				//根据redirect判断是否需要拼接desturl
				if (cbidobject.redirect)
				{
					if (adxtemp.cturl.size() > 0)
					{
						int lenout;
						string strtemp = curl;
						char *enurl = urlencode(strtemp.c_str(), strtemp.size(), &lenout);
                        curl = adxtemp.cturl[0] + trace_url_par; 
						++k;
						curl += "&curl=" + string(enurl, lenout);
						free(enurl);
					}
				}

				//curl需替换BID宏MAPID宏
				//adomain为落地页地址
				bid->set_adomain(curl);
				//广告家只支持一个点击检测地址
				string cturl = "";
				if (k < adxtemp.cturl.size())
				{
					cturl = adxtemp.cturl[k] + trace_url_par;
				}
				else if (cbidobject.cmonitorurl.size() >0)
				{
					cturl = cbidobject.cmonitorurl[0];
				}

				if (cturl.size() > 0)
				{//curl为点击检测地址
					bid->set_curl(cturl);
				}

				switch (cbidobject.ctype)
				{
				case 2:
				case 3:
				{
					bid->set_action("install");
					break;
				}
				default:
					bid->set_action("click");
				}
			}
		}
	}
	else
	{
		bidresponse.set_nbr(UNKNOWN_ERROR);
	}

	return;
}

static bool filter_cbidobject_callback(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if (price - crequest.imp[0].bidfloor - 0.01 > 0.000001)
		return true;

	return false;
}

static bool process_crequest_callback(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = adxtemplate.ratio;

	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
		return false;

	for (int i = 0; i < crequest.imp.size(); ++i)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];
		cimp.bidfloor /= ratio;
	}

	return true;
}

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int err = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	uint32_t sendlength = 0;
	COM_REQUEST crequest;//common msg request
	COM_RESPONSE cresponse;
	ADXTEMPLATE adxtemp;
	struct timespec ts1, ts2;
	long lasttime;
	string str_ferrcode = "";
	BidRequest bidrequest;
	BidResponse bidresponse;
	char *data = NULL;
	int length = 0;

	if (recvdata == NULL)
	{
		err = E_BAD_REQUEST;
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		err = E_BAD_REQUEST;
		goto exit;
	}

	getTime(&ts1);
	init_com_message_request(&crequest);

	err = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
	if (!err)
	{
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		va_cout("parse data from array failed!");
		err = E_BAD_REQUEST;
		goto exit;
	}

	err = parseGoyooRequest(bidrequest, crequest, bidresponse);
	if (err == E_BAD_REQUEST)
	{
		va_cout("bad request");
		goto exit;
	}

	flow_bid.set_req(crequest);

	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s, getRegionCode ip:%s location:%d failed!", 
			crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		err = E_REQUEST_DEVICE_IP;
	}
	else if (err == E_SUCCESS)
	{
		init_com_message_response(&cresponse);

		ferrcode = bid_filter_response(ctx, index, crequest, NULL, NULL, filter_cbidobject_callback, NULL, &cresponse, &adxtemp);
		err = ferrcode.errcode;
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		flow_bid.set_bid_flow(ferrcode);
		g_ser_log.send_bid_log(index, crequest, cresponse, err);
		setGoyooJsonResponse(crequest, cresponse, adxtemp, bidresponse, err);
	}

	length = bidresponse.ByteSize();
	data = (char *)calloc(1, sizeof(char) * (length + 1));
	if (data == NULL)
	{
		va_cout("allocate memory failure!");
		writeLog(g_logid_local, LOGERROR, "allocate memory failure!");
		senddata = "Status: 204 OK\r\n\r\n";
		err = E_MALLOC;
		goto exit;
	}

	bidresponse.SerializeToArray(data, length);

	senddata = string("Content-Type: application/x-protobuf\r\nContent-Length: ") + 
		intToString(length) + "\r\n\r\n" + string(data, length);

	cout << "senddata :" << senddata << endl;
	writeLog(g_logid_local, LOGINFO, string(data, length));

	if (err == E_SUCCESS)
	{
		numcount[index]++;
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", 
			cresponse.id.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(), 
			crequest.device.deviceid.c_str(), crequest.device.deviceidtype, 
			cresponse.seatbid[0].bid[0].price);
		if (numcount[index] % 1000 == 0)
		{
			string str_response;
			google::protobuf::TextFormat::PrintToString(bidresponse, &str_response);
			writeLog(g_logid_local, LOGDEBUG, "response=%s", str_response.c_str());
			numcount[index] = 0;
		}
	}

	bidresponse.Clear();
	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x", crequest.id.c_str(), lasttime, err);

exit:
	if (data != NULL)
	{
		free(data);
		data = NULL;
	}
	flow_bid.set_err(err);
	return err;
}
