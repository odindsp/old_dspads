/*
 * inmobi_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/tcp.h"
#include "../../common/json_util.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/redisoperation.h"
#include "../../common/getlocation.h"
#include "../../common/type.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/server_log.h"
#include "../../common/bid_filter_dump.h"
#include "letv_response.h"

#define ADS_AMOUNT 20

extern uint64_t g_logid, g_logid_local, g_logid_response;
extern MD5_CTX hash_ctx;
extern uint64_t geodb;
extern map<string, vector<uint32_t> > adv_cat_table_adxi;
extern bool fullreqrecord;

extern map<string, uint16_t> letv_make_table;

void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

static bool callback_process_crequest(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = 0.0;

	ratio = adxtemplate.ratio;
	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
		return false;

	for (int i = 0; i < crequest.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];
		cimp.bidfloor /= ratio;
	}

	return true;
}

static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if (price >= crequest.imp[0].bidfloor)
		return true;
	else
		return false;
}

static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &exts)
{
	bool filter = true;
	if (exts.id == "")
	{
		filter = false;
		goto exit;
	}

exit:
	return filter;
}

// parse bid request
int parseLetvRequest(char *data, COM_REQUEST &request)
{
	json_t *root = NULL;
	json_t *label = NULL;
	int err = E_SUCCESS;

	if (data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);
	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseLetvRequest root is NULL!");
		return E_BAD_REQUEST;
	}

	if (fullreqrecord)
		writeLog(g_logid_local, LOGDEBUG, "letv request:%s", data);

	if ((label = json_find_first_label(root, "id")) != NULL)
		request.id = label->child->text;
	else
	{
		err = E_REQUEST_NO_BID;
		goto exit;
	}

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;

		if (imp_value != NULL)
		{
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);
			if ((label = json_find_first_label(imp_value, "id")) != NULL)
				imp.id = label->child->text;

			/*	if((label = json_find_first_label(imp_value, "adzoneid")) != NULL)
					imp.adzoneid = atoi(label->child->text); */

			if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL)
			{
				imp.bidfloor = atof(label->child->text) / 100.0;//muti ratio, letv is 1
				imp.bidfloorcur = "CNY";
			}

			int display = 0;
			if ((label = json_find_first_label(imp_value, "display")) != NULL && label->child->type == JSON_NUMBER)
			{
				display = atoi(label->child->text);
			}

			if ((label = json_find_first_label(imp_value, "banner")) != NULL)
			{
				json_t *banner_value = label->child;

				imp.type = IMP_BANNER;
				if (banner_value != NULL)
				{
					if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.w = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);
				}
			}

			if ((label = json_find_first_label(imp_value, "pmp")) != NULL)
			{
				writeLog(g_logid_local, LOGDEBUG, "letv pmp request:%s", data);
				err = E_REQUEST_AT;
				/*json_t *pmp_value = label->child;
				uint16_t private_auction = 0;

				if(pmp_value != NULL)
				{
				if((label = json_find_first_label(pmp_value, "private_auction")) != NULL)
				{
				private_auction = atoi(label->child->text);
				//表示这部分流量会和其他 DSP 竞价
				if(private_auction == 0)
				{
				err = E_INVALID_PARAM;
				}
				}
				if((label = json_find_first_label(pmp_value, "deals")) != NULL)
				{
				json_t *deal = label->child->child;
				while(deal)
				{
				if((label = json_find_first_label(deal, "id")) != NULL)
				{

				}
				if((label = json_find_first_label(deal, "bidfloor")) != NULL)
				{

				}
				if((label = json_find_first_label(deal, "wseat")) != NULL)
				{

				}
				}
				}
				}*/
			}

			if ((label = json_find_first_label(imp_value, "video")) != NULL)
			{
				json_t *video_value = label->child;

				imp.type = IMP_VIDEO;
				imp.video.is_limit_size = false;
				if (video_value != NULL)
				{
					if ((label = json_find_first_label(video_value, "mime")) != NULL)
					{
						json_t *temp = label->child->child;
						uint8_t video_type = VIDEOFORMAT_UNKNOWN;

						while (temp != NULL)
						{
							video_type = VIDEOFORMAT_UNKNOWN;
							if (strcmp(temp->text, "video\\/x-flv") == 0)
								video_type = VIDEOFORMAT_X_FLV;
							else if (strcmp(temp->text, "video\\/mp4") == 0)
								video_type = VIDEOFORMAT_MP4;

							imp.video.mimes.insert(video_type);
							temp = temp->next;
						}
					}
					switch (display)
					{
					case 0:
					{
						display = DISPLAY_PAGE;
						break;
					}
					case 1:
					{
						display = DISPLAY_DRAW_CURTAIN;
						break;
					}
					case 2:
					{
						display = DISPLAY_FRONT_PASTER;
						break;
					}
					case 3://标版广告
					{
						break;
					}
					case 4:
					{
						display = DISPLAY_MIDDLE_PASTER;
						break;
					}
					case 5:
					{
						display = DISPLAY_BACK_PASTER;
						break;
					}
					case 6:
					{
						display = DISPLAY_PAUSE;
						break;
					}
					case 7:
					{
						display = DISPLAY_SUPERNATANT;
						break;
					}
					default:
						break;
					}
					imp.video.display = display;

					if ((label = json_find_first_label(video_value, "minduration")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.minduration = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "maxduration")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.maxduration = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.w = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.h = atoi(label->child->text);
				}
			}

			if (JSON_FIND_LABEL_AND_CHECK(imp_value, label, "ext", JSON_OBJECT))
			{
				json_t *tmp = label->child;
				if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "excluded_ad_industry", JSON_ARRAY))
				{
					//需通过映射,传递给后端
					json_t *temp = label->child->child;
					while (temp != NULL)
					{
						vector<uint32_t> &v_cat = adv_cat_table_adxi[temp->text];
						for (uint32_t i = 0; i < v_cat.size(); ++i)
							request.bcat.insert(v_cat[i]);

						temp = temp->next;
					}
				}
			}
			request.imp.push_back(imp);
		}
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if ((label = json_find_first_label(app_child, "name")) != NULL)
		{
			request.app.name = label->child->text;
			request.app.id = label->child->text;
		}

		if ((label = json_find_first_label(app_child, "ext")) != NULL)
		{
			json_t *ext_value = label->child;

			if (ext_value != NULL)
			{
				/*if((label = json_find_first_label(ext_value, "sdk")) != NULL)
					request.app.ext.sdk = label->child->text;

					if((label = json_find_first_label(ext_value, "market")) != NULL && label->child->type == JSON_NUMBER)
					request.app.ext.market = atoi(label->child->text);
					*/
				//if((label = json_find_first_label(ext_value, "appid")) != NULL)
				//	request.app.id = label->child->text;

				if ((label = json_find_first_label(ext_value, "cat")) != NULL)
				{
					if (label->child->text != "")
						request.app.cat.insert(atoi(label->child->text));
				}

				/*if((label = json_find_first_label(ext_value, "tag")) != NULL)
					request.app.ext.tag = label->child->text;*/
			}
		}

		/*if((label = json_find_first_label(app_child, "content")) != NULL)
		{
		json_t *content_value = label->child;

		if(content_value != NULL)
		{
		if((label = json_find_first_label(content_value, "channel")) != NULL)
		request.app.content.channel = label->child->text;

		if((label = json_find_first_label(content_value, "cs")) != NULL)
		request.app.content.cs = label->child->text;
		}
		}*/
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;

		if ((label = json_find_first_label(device_child, "ua")) != NULL)
		{
			request.device.ua = label->child->text;
			replaceMacro(request.device.ua, "\\", "");
		}

		if ((label = json_find_first_label(device_child, "ip")) != NULL)
		{
			request.device.ip = label->child->text;
			if (request.device.ip.size() > 0)
			{
				int location = getRegionCode(geodb, request.device.ip);
				request.device.location = location;
			}
		}

		if ((label = json_find_first_label(device_child, "geo")) != NULL)
		{
			json_t *geo_value = label->child;

			if ((label = json_find_first_label(geo_value, "lat")) != NULL)
				request.device.geo.lat = atof(label->child->text);

			if ((label = json_find_first_label(geo_value, "lon")) != NULL)
				request.device.geo.lon = atof(label->child->text);

			/*if((label = json_find_first_label(geo_value, "ext")) != NULL)
			{
			json_t *ext_value = label->child;

			if(ext_value != NULL)
			{
			if((label = json_find_first_label(ext_value, "accuray")) != NULL  && label->child->type == JSON_NUMBER)
			request.device.geo.ext.accuray= atoi(label->child->text);
			}
			}*/
		}

		if ((label = json_find_first_label(device_child, "carrier")) != NULL)
		{
			uint8_t carrier = CARRIER_UNKNOWN;
			if (!strcmp(label->child->text, "China Mobile"))
				carrier = CARRIER_CHINAMOBILE;
			else if (!strcmp(label->child->text, "China Unicom"))
				carrier = CARRIER_CHINAUNICOM;
			else if (!strcmp(label->child->text, "China Telecom"))
				carrier = CARRIER_CHINATELECOM;

			request.device.carrier = carrier;
		}

		//if((label = json_find_first_label(device_child, "language")) != NULL)
		//	request.device.language = label->child->text;

		if ((label = json_find_first_label(device_child, "make")) != NULL)
		{
			string s_make = label->child->text;
			if (s_make != "")
			{
				map<string, uint16_t>::iterator it;

				request.device.makestr = s_make;
				for (it = letv_make_table.begin(); it != letv_make_table.end(); ++it)
				{
					if (s_make.find(it->first) != string::npos)
					{
						request.device.make = it->second;
						break;
					}
				}
			}
		}

		if ((label = json_find_first_label(device_child, "model")) != NULL)
			request.device.model = label->child->text;

		if ((label = json_find_first_label(device_child, "os")) != NULL)
		{
			uint8_t os = 0;

			if (!strcasecmp(label->child->text, "ANDROID"))
			{
				os = OS_ANDROID;
				request.is_secure = 0;
			}
			else if (!strcasecmp(label->child->text, "IOS"))
			{
				os = OS_IOS;
				//request.is_secure = 1;
			}
			else if (!strcasecmp(label->child->text, "WP"))
				os = OS_WINDOWS;
			else
				os = OS_UNKNOWN;

			request.device.os = os;
		}

		/* Now support origial imei and deviceid */
		if ((label = json_find_first_label(device_child, "did")) != NULL)  //device id
		{
			string did = label->child->text;
			if (did != "")
				request.device.dids.insert(make_pair(DID_IMEI, did));
		}

		if ((label = json_find_first_label(device_child, "dpid")) != NULL)
		{
			string dpid = label->child->text;
			if (dpid != "")
			{
				uint8_t dpidtype = DPID_UNKNOWN;

				switch (request.device.os)
				{
				case OS_ANDROID:
				{
					request.device.dpids.insert(make_pair(DPID_ANDROIDID, dpid));
					break;
				}
				//ios UDID
				/*case OS_IOS:
					{
					dpidtype = DPID_IDFA;
					break;
					}*/
				case OS_WINDOWS:
				{
					request.device.dpids.insert(make_pair(DPID_OTHER, dpid));
					break;
				}
				default:
					break;
				}


			}
		}

		if ((label = json_find_first_label(device_child, "osv")) != NULL)
			request.device.osv = label->child->text;

		/*if((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
			request.device.js = atoi(label->child->text);
			*/

		if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
		{
			uint8_t connectiontype = CONNECTION_UNKNOWN;

			connectiontype = atoi(label->child->text);
			switch (connectiontype)
			{
			case 2:
				connectiontype = CONNECTION_WIFI;
				break;
			case 3:
				connectiontype = CONNECTION_CELLULAR_UNKNOWN;
				break;
			case 4:
				connectiontype = CONNECTION_CELLULAR_2G;
				break;
			case 5:
				connectiontype = CONNECTION_CELLULAR_3G;
				break;
			case 6:
				connectiontype = CONNECTION_CELLULAR_4G;
				break;
			default:
				connectiontype = CONNECTION_UNKNOWN;
				break;
			}

			request.device.connectiontype = connectiontype;
		}

		if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
		{
			uint8_t devicetype = 0, device_new_type = 0;

			devicetype = atoi(label->child->text);
			switch (devicetype)
			{
			case 0:
				device_new_type = DEVICE_PHONE;
				break;
			case 1:
				device_new_type = DEVICE_TABLET;
				break;
			case 3:
				device_new_type = DEVICE_TV;
				break;
			default:
				device_new_type = DEVICE_UNKNOWN;
				break;
			}

			request.device.devicetype = device_new_type;
		}


		if ((label = json_find_first_label(device_child, "ext")) != NULL)
		{
			json_t *ext_value = label->child;

			if ((label = json_find_first_label(ext_value, "idfa")) != NULL)
			{
				string idfa = label->child->text;;
				if (idfa != "")
					request.device.dpids.insert(make_pair(DPID_IDFA, idfa));
			}

			if ((label = json_find_first_label(ext_value, "mac")) != NULL)
			{
				string mac = label->child->text;
				if (mac != "")
					request.device.dids.insert(make_pair(DID_MAC, mac));
			}

			if ((label = json_find_first_label(ext_value, "macmd5")) != NULL)
			{
				string macmd5 = label->child->text;
				if (macmd5 != "")
					request.device.dids.insert(make_pair(DID_MAC_MD5, macmd5));
			}

			/*if((label = json_find_first_label(ext_value, "ssid")) != NULL)
				request.device.ext.ssid = label->child->text;

				if((label = json_find_first_label(ext_value, "w")) != NULL && label->child->type == JSON_NUMBER)
				request.device.ext.w = atoi(label->child->text);

				if((label = json_find_first_label(ext_value, "h")) != NULL && label->child->type == JSON_NUMBER)
				request.device.ext.h = atoi(label->child->text);

				if((label = json_find_first_label(ext_value, "brk")) != NULL && label->child->type == JSON_NUMBER)
				request.device.ext.brk = atoi(label->child->text);

				if((label = json_find_first_label(ext_value, "ts")) != NULL && label->child->type == JSON_NUMBER)
				request.device.ext.ts = atoi(label->child->text);
				*/
			if ((label = json_find_first_label(ext_value, "interstitial")) != NULL && label->child->type == JSON_NUMBER)
			{
				if (request.imp.size() > 0)
				{
					request.imp[0].ext.instl = atoi(label->child->text);
				}
			}
		}
	}

	if ((label = json_find_first_label(root, "user")) != NULL)
	{
		json_t *user_child = label->child;

		if ((label = json_find_first_label(user_child, "id")) != NULL)
			request.user.id = label->child->text;

		if ((label = json_find_first_label(user_child, "gender")) != NULL)
		{
			string gender = label->child->text;
			if (gender == "M")
				request.user.gender = GENDER_MALE;
			else if (gender == "F")
				request.user.gender = GENDER_FEMALE;
			else
				request.user.gender = GENDER_UNKNOWN;
		}

		if ((label = json_find_first_label(user_child, "yob")) != NULL && label->child->type == JSON_NUMBER)
			request.user.yob = atoi(label->child->text);

	}

exit:
	if (root != NULL)
		json_free_value(&root);

	return err;
}

// response to adx COM_RESPONSE &comresponse, MESSAGERESPONSE response
bool setLetvJsonResponse(COM_REQUEST comrequest, COM_RESPONSE &comresponse, ADXTEMPLATE *adxinfo, string &response_out)
{
	string trace_url_par = string("&") + get_trace_url_par(comrequest, comresponse);
	char *text = NULL;
	json_t *root = NULL, *label_seatbid = NULL,
		*label_bid = NULL, *array_seatbid = NULL,
		*array_bid = NULL, *entry_seatbid = NULL,
		*entry_bid = NULL, *entry_ext = NULL, *label_ext = NULL;

	bool bresponse = true;
	uint32_t i, j, k;

	root = json_new_object();
	jsonInsertString(root, "id", comresponse.id.c_str());
	jsonInsertString(root, "bidid", comresponse.bidid.c_str());

	if (comresponse.seatbid.size() > 0)
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < comresponse.seatbid.size(); ++i)
		{
			COM_SEATBIDOBJECT *seatbid = &comresponse.seatbid[i];
			entry_seatbid = json_new_object();

			if (seatbid->bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid->bid.size(); ++j)
				{
					COM_BIDOBJECT &bid = seatbid->bid[j];
					entry_bid = json_new_object();
					label_ext = json_new_string("ext");
					entry_ext = json_new_object();

					double price = (double)bid.price * 100;// 分/CPM
					jsonInsertString(entry_bid, "id", bid.impid.c_str());
					jsonInsertString(entry_bid, "impid", bid.impid.c_str());
					jsonInsertFloat(entry_bid, "price", price);

					string nurl = adxinfo->nurl + trace_url_par;
					jsonInsertString(entry_bid, "nurl", nurl.c_str());

					jsonInsertString(entry_bid, "adm", bid.sourceurl.c_str());//source material url

					string curl = bid.curl;

					jsonInsertString(entry_ext, "ldp", curl.c_str());//curl 

					/* Imp Url */
					if (adxinfo->iurl.size() > 0)
					{
						json_t *ext_pm, *ext_pm_entry, *value_pm;

						ext_pm = json_new_string("pm");
						ext_pm_entry = json_new_array();

						for (j = 0; j < adxinfo->iurl.size(); ++j)
						{
							string iurl = adxinfo->iurl[j] + trace_url_par;
							value_pm = json_new_string(iurl.c_str());
							json_insert_child(ext_pm_entry, value_pm);
						}

						for (int k = 0; k < bid.imonitorurl.size(); k++)
						{
							if (bid.imonitorurl[k].c_str())
							{
								value_pm = json_new_string(bid.imonitorurl[k].c_str());
								json_insert_child(ext_pm_entry, value_pm);
							}
						}

						json_insert_child(ext_pm, ext_pm_entry);
						json_insert_child(entry_ext, ext_pm);
					}

					/* Click Url */
					if (adxinfo->cturl.size() > 0)
					{
						json_t *ext_cm, *ext_cm_entry, *value_cm;

						ext_cm = json_new_string("cm");
						ext_cm_entry = json_new_array();

						for (j = 0; j < adxinfo->cturl.size(); ++j)
						{
							string cturl = adxinfo->cturl[j] + trace_url_par;
							value_cm = json_new_string(cturl.c_str());
							json_insert_child(ext_cm_entry, value_cm);
						}

						for (int k = 0; k < bid.cmonitorurl.size(); k++)
						{
							if (bid.cmonitorurl[k] != "")
							{
								value_cm = json_new_string(bid.cmonitorurl[k].c_str());
								json_insert_child(ext_cm_entry, value_cm);
							}
						}

						json_insert_child(ext_cm, ext_cm_entry);
						json_insert_child(entry_ext, ext_cm);
					}

					switch (bid.ftype)
					{
					case ADFTYPE_IMAGE_PNG:
						jsonInsertString(entry_ext, "type", "image/png");
						break;
					case ADFTYPE_IMAGE_JPG:
						jsonInsertString(entry_ext, "type", "image/jpeg");
						break;
					case ADFTYPE_IMAGE_GIF:
						jsonInsertString(entry_ext, "type", "image/gif");
						break;
					case ADFTYPE_VIDEO_FLV:
						jsonInsertString(entry_ext, "type", "video/x-flv");
						break;
					case ADFTYPE_VIDEO_MP4:
						jsonInsertString(entry_ext, "type", "video/mp4");
						break;
					default:
						if (bid.type == ADTYPE_FLASH)
							jsonInsertString(entry_ext, "type", "application/x-shockwave-flash");
						else
							jsonInsertString(entry_ext, "type", "unknown");
						break;
					}

					json_insert_child(label_ext, entry_ext);
					json_insert_child(entry_bid, label_ext);
				}
				json_insert_child(array_bid, entry_bid);
			}
			json_insert_child(label_bid, array_bid);
			json_insert_child(entry_seatbid, label_bid);

			json_insert_child(array_seatbid, entry_seatbid);
		}
		json_insert_child(label_seatbid, array_seatbid);
		json_insert_child(root, label_seatbid);
	}
	else
	{
		bresponse = false;
		goto exit;
	}

exit:
	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		response_out = text;
		free(text);
	}

	if (root != NULL)
		json_free_value(&root);

	return bresponse;
}

int letvResponse(IN uint8_t index, IN uint64_t ctx, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int errcode = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST comrequest;
	COM_RESPONSE comresponse;
	ADXTEMPLATE adxinfo;
	string str_ferrcode = "";
	struct timespec ts1, ts2;
	long lasttime = 0;

	getTime(&ts1);
	bool parsesuccess = false;

	if ((recvdata == NULL) || (ctx == 0))
	{
		errcode = E_BID_PROGRAM;
		writeLog(g_logid_local, LOGERROR, string("recvdata or ctx is NULL"));
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		errcode = E_BID_PROGRAM;
		writeLog(g_logid_local, LOGERROR, string("recvdata->data is null or recvdata->length is 0"));
		goto exit;
	}

	/* parse bid request,in the parseInmobiRequest doesn't parse user*/
	init_com_message_request(&comrequest);

	errcode = parseLetvRequest(recvdata->data, comrequest);
	if (errcode == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);
		goto exit;
	}
	flow_bid.set_req(comrequest);

	if (comrequest.device.location == 0 || comrequest.device.location > 1156999999 || comrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s, getRegionCode ip:%s location:%d failed!", 
			comrequest.id.c_str(), comrequest.device.ip.c_str(), comrequest.device.location);
		errcode = E_REQUEST_DEVICE_IP;
	}
	else if (errcode == E_SUCCESS)
	{
		ferrcode = bid_filter_response(ctx, index, comrequest, callback_process_crequest,
			NULL, callback_check_price, filter_bid_ext, &comresponse, &adxinfo);
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		flow_bid.set_bid_flow(ferrcode);
		errcode = ferrcode.errcode;
		g_ser_log.send_bid_log(index, comrequest, comresponse, errcode);
	}

exit:
	if (errcode == E_SUCCESS)
	{
		string outputdata = "";

		setLetvJsonResponse(comrequest, comresponse, &adxinfo, outputdata);

		senddata = string("Content-Type: application/json\r\nContent-Length: ")
			+ intToString(outputdata.size()) + "\r\n\r\n" + outputdata;

		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", comrequest.id.c_str(),
			comresponse.seatbid[0].bid[0].mapid.c_str(), comrequest.device.deviceid.c_str(),
			comrequest.device.deviceidtype, comresponse.seatbid[0].bid[0].price);
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, comrequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());

	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);

	flow_bid.set_err(errcode);
	return errcode;
}
