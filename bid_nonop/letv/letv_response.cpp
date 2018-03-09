/*
 * inmobi_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <algorithm>
#include "../../common/json_util.h"
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

	for (int i = 0; i < crequest.imp.size(); i++ )
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];
		cimp.bidfloor /= ratio;
	}

	return true;
}

static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price)
{
	if(price >= crequest.imp[0].bidfloor)
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

	if(data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);
	if(root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseLetvRequest root is NULL!");
		return E_BAD_REQUEST;
	}

	if(fullreqrecord)
		writeLog(g_logid_local, LOGDEBUG, "letv request:%s", data);

	if((label = json_find_first_label(root, "id")) != NULL)
		request.id = label->child->text;
	else
	{
		err = E_BAD_REQUEST;
		goto exit;
	}

	if((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;

		if(imp_value != NULL)
		{
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);
			if((label = json_find_first_label(imp_value, "id")) != NULL)
				imp.id = label->child->text; 
			
		/*	if((label = json_find_first_label(imp_value, "adzoneid")) != NULL)
				imp.adzoneid = atoi(label->child->text); */
	
			if((label = json_find_first_label(imp_value, "bidfloor")) != NULL)
			{
				imp.bidfloor = atof(label->child->text) / 100.0;//muti ratio, letv is 1
				imp.bidfloorcur = "CNY";
			}

			int display = 0;
			if((label = json_find_first_label(imp_value, "display")) != NULL && label->child->type == JSON_NUMBER)
			{
				display = atoi(label->child->text);
			}

			if((label = json_find_first_label(imp_value, "banner")) != NULL)
			{
				json_t *banner_value = label->child;

				imp.type = IMP_BANNER;
				if(banner_value != NULL)
				{
					if((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.w = atoi(label->child->text);
						
					if((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);
				}
			}

			if((label = json_find_first_label(imp_value, "pmp")) != NULL)
			{
				writeLog(g_logid_local, LOGDEBUG, "letv pmp request:%s", data);
				err = E_INVALID_PARAM;
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

			if((label = json_find_first_label(imp_value, "video")) != NULL)
			{
				json_t *video_value = label->child;

				imp.type = IMP_VIDEO;

				if(video_value != NULL)
				{
					if((label = json_find_first_label(video_value, "mime")) != NULL)
					{
						json_t *temp = label->child->child;
						uint8_t video_type = VIDEOFORMAT_UNKNOWN;
						string mime;

						while(temp != NULL)
						{
							video_type = VIDEOFORMAT_UNKNOWN;
							mime = temp->text;
							replaceMacro(mime, "\\", "");
							if (mime == "video/x-flv")
								video_type = VIDEOFORMAT_X_FLV;
							else if (mime == "video/mp4")
								video_type = VIDEOFORMAT_MP4;

							imp.video.mimes.insert(video_type);
							temp = temp->next;			
						}
					}
					switch(display)
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

					if((label = json_find_first_label(video_value, "minduration")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.minduration = atoi(label->child->text);

					if((label = json_find_first_label(video_value, "maxduration")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.maxduration = atoi(label->child->text);

					if((label = json_find_first_label(video_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.w = atoi(label->child->text);

					if((label = json_find_first_label(video_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.h = atoi(label->child->text);
				}
			}

			if(JSON_FIND_LABEL_AND_CHECK(imp_value, label, "ext", JSON_OBJECT))
			{
				json_t *tmp = label->child;
				if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "excluded_ad_industry", JSON_ARRAY))
				{
					//需通过映射,传递给后端
					json_t *temp = label->child->child;
					while (temp != NULL)
					{		
						vector<uint32_t> &v_cat = adv_cat_table_adxi[temp->text];
						for(uint32_t i = 0; i < v_cat.size(); ++i)
							request.bcat.insert(v_cat[i]);

						temp = temp->next;
					}
				}
			}
			request.imp.push_back(imp);
		}
	}

	if((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if((label = json_find_first_label(app_child, "name")) != NULL)
			request.app.name = label->child->text;

		if((label = json_find_first_label(app_child, "ext")) != NULL)
		{
			json_t *ext_value = label->child;

			if(ext_value != NULL)
			{
				/*if((label = json_find_first_label(ext_value, "sdk")) != NULL)
					request.app.ext.sdk = label->child->text;

				if((label = json_find_first_label(ext_value, "market")) != NULL && label->child->type == JSON_NUMBER)
					request.app.ext.market = atoi(label->child->text);
					*/
				if((label = json_find_first_label(ext_value, "appid")) != NULL)
					request.app.id = label->child->text;

				if((label = json_find_first_label(ext_value, "cat")) != NULL)
				{
					if(label->child->text != "")
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

	if((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;

		if((label = json_find_first_label(device_child, "ua")) != NULL)
		{
			request.device.ua = label->child->text;
			replaceMacro(request.device.ua, "\\", "");
		}

		if((label = json_find_first_label(device_child, "ip")) != NULL)
		{
			request.device.ip = label->child->text;
			if(request.device.ip.size() > 0)
			{
				int location = getRegionCode(geodb, request.device.ip);
				request.device.location = location;
			}
		}

		if((label = json_find_first_label(device_child, "geo")) != NULL)
		{
			json_t *geo_value = label->child;

			if((label = json_find_first_label(geo_value, "lat")) != NULL)
				request.device.geo.lat = atof(label->child->text);

			if((label = json_find_first_label(geo_value, "lon")) != NULL)
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

		if((label = json_find_first_label(device_child, "carrier")) != NULL)
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

		if((label = json_find_first_label(device_child, "make")) != NULL)
		{
			size_t position;
			map<string, uint16_t>::iterator it;

			string s_make = string(label->child->text);

			for (it = letv_make_table.begin();it != letv_make_table.end();++it)
			{
				position = s_make.find(it->first);
				if (position != string::npos)
				{
					request.device.make = it->second;
					break;
				}
			}
			if(it == letv_make_table.end())
			{
#ifdef DEBUG
				writeLog(g_logid_local, LOGDEBUG, "%s,not find make:%s", request.id.c_str(), s_make.c_str());
#endif
			}
		}

		if((label = json_find_first_label(device_child, "model")) != NULL)
			request.device.model = label->child->text;

		if((label = json_find_first_label(device_child, "os")) != NULL)
		{
			uint8_t os = 0;
			
			if (!strcasecmp(label->child->text, "ANDROID"))
				os = OS_ANDROID;
			else if (!strcasecmp(label->child->text, "IOS"))
				os = OS_IOS;
			else if (!strcasecmp(label->child->text, "WP"))
				os = OS_WINDOWS;
			else
				os = OS_UNKNOWN;

			request.device.os = os;
		}

		/* Now support origial imei and deviceid */
		if((label = json_find_first_label(device_child, "did")) != NULL)  //device id
		{
			string did = label->child->text;
			if(did != "")
				request.device.dids.insert(make_pair(DID_IMEI, did));
		}

		if((label = json_find_first_label(device_child, "dpid")) != NULL)
		{	
			string dpid = label->child->text;
			if(dpid != "")
			{
				uint8_t dpidtype = DPID_UNKNOWN;

				switch(request.device.os)
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

		if((label = json_find_first_label(device_child, "osv")) != NULL)
			request.device.osv = label->child->text;

		/*if((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
			request.device.js = atoi(label->child->text);
		*/

		if((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
		{
			uint8_t connectiontype = CONNECTION_UNKNOWN;

			connectiontype = atoi(label->child->text);
			switch(connectiontype)
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

		if((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
		{
			uint8_t devicetype = 0, device_new_type = 0;

			devicetype = atoi(label->child->text);
			switch(devicetype)
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

		
		if((label = json_find_first_label(device_child, "ext")) != NULL)
		{
			json_t *ext_value = label->child;

			if((label = json_find_first_label(ext_value, "idfa")) != NULL)
			{
				string idfa =  label->child->text;;
				if(idfa != "")
					request.device.dpids.insert(make_pair(DPID_IDFA, idfa));
			}

			if((label = json_find_first_label(ext_value, "mac")) != NULL)
			{
				string mac = label->child->text;
				if(mac != "")
					request.device.dids.insert(make_pair(DID_MAC, mac));
			}

			if((label = json_find_first_label(ext_value, "macmd5")) != NULL)
			{
				string macmd5 = label->child->text;
				if(macmd5 != "")
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
			if((label = json_find_first_label(ext_value, "interstitial")) != NULL && label->child->type == JSON_NUMBER)
			{
				if(request.imp.size() > 0)
				{
					request.imp[0].ext.instl = atoi(label->child->text);
				}
			}
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);

	return err;
}

void replace_url(IN COM_REQUEST crequest, IN string mapid, INOUT string &url)
{
	replaceMacro(url, "#BID#", crequest.id.c_str());
	replaceMacro(url, "#MAPID#", mapid.c_str());
	replaceMacro(url, "#DEVICEID#", crequest.device.deviceid.c_str());
	replaceMacro(url, "#DEVICEIDTYPE#", intToString(crequest.device.deviceidtype).c_str());
}

// response to adx COM_RESPONSE &comresponse, MESSAGERESPONSE response
bool setLetvJsonResponse(COM_REQUEST comrequest, COM_RESPONSE &comresponse, ADXTEMPLATE *adxinfo, string &response_out)
{
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

	if(!comresponse.seatbid.empty())
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid =  json_new_array();

		for(i = 0; i < comresponse.seatbid.size(); i++)
		{
			COM_SEATBIDOBJECT *seatbid = &comresponse.seatbid[i];
			entry_seatbid = json_new_object();

			if(!seatbid->bid.empty() && seatbid->bid.size())
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for(j = 0;j < seatbid->bid.size();j++)
				{
					COM_BIDOBJECT *bid = &seatbid->bid[j];
					entry_bid = json_new_object();
					label_ext = json_new_string("ext");
					entry_ext = json_new_object();
					
					double price = (double)bid->price * 100;// 分/CPM
					jsonInsertString(entry_bid, "id", bid->impid.c_str());
					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					jsonInsertFloat(entry_bid, "price", price);

					replace_url(comrequest, bid->adid, adxinfo->nurl);
					jsonInsertString(entry_bid, "nurl", adxinfo->nurl.c_str());

					jsonInsertString(entry_bid, "adm", bid->sourceurl.c_str());//source material url

					string curl = bid->curl;
					if(curl.find("__ANDROIDID__") < curl.size())
					{
						replaceNielsenMonitor(hash_ctx, comrequest.device, curl);
					}
					if(curl.find("__AndroidID__") < curl.size())
					{
						replaceAdmasterMonitor(hash_ctx, comrequest.device, curl, comrequest.id);
					}
					replace_url(comrequest, bid->adid, curl);
					jsonInsertString(entry_ext, "ldp", curl.c_str());//curl 

					/* Imp Url */
					if(!adxinfo->iurl.empty() && adxinfo->iurl.size() > 0)
					{
						json_t *ext_pm, *ext_pm_entry, *value_pm;

						ext_pm = json_new_string("pm");
						ext_pm_entry = json_new_array();

						for(j = 0;j < adxinfo->iurl.size();++j)
						{
							replace_url(comrequest, bid->adid, adxinfo->iurl[j]);

							value_pm = json_new_string(adxinfo->iurl[j].c_str());
							json_insert_child(ext_pm_entry, value_pm);
						}

						for (int k = 0;k < bid->imonitorurl.size();k++)
						{
							if (bid->imonitorurl[k].c_str())
							{
								if (bid->imonitorurl[k].find("__ANDROIDID__") < bid->imonitorurl[k].size())
								{
									replaceNielsenMonitor(hash_ctx, comrequest.device, bid->imonitorurl[k]);
								}
								if (bid->imonitorurl[k].find("__AndroidID__") < bid->imonitorurl[k].size())
								{
									replaceAdmasterMonitor(hash_ctx, comrequest.device,  bid->imonitorurl[k], comrequest.id);
								}

								value_pm = json_new_string(bid->imonitorurl[k].c_str());
								json_insert_child(ext_pm_entry, value_pm);
							}
						}

						json_insert_child(ext_pm, ext_pm_entry);
						json_insert_child(entry_ext, ext_pm);
					}

					/* Click Url */
					if(!adxinfo->cturl.empty() && adxinfo->cturl.size() > 0)
					{
						json_t *ext_cm, *ext_cm_entry, *value_cm;

						ext_cm = json_new_string("cm");
						ext_cm_entry = json_new_array();

						for(j = 0;j < adxinfo->cturl.size();++j)
						{
							replace_url(comrequest, bid->adid, adxinfo->cturl[j]);
							value_cm = json_new_string(adxinfo->cturl[j].c_str());
							json_insert_child(ext_cm_entry, value_cm);
						}

						for (int k = 0;k < bid->cmonitorurl.size();k++)
						{
							if (bid->cmonitorurl[k].c_str())
							{
								if (bid->cmonitorurl[k].find("__ANDROIDID__") < bid->cmonitorurl[k].size())
								{
									replaceNielsenMonitor(hash_ctx, comrequest.device, bid->cmonitorurl[k]);
								}
								if (bid->cmonitorurl[k].find("__AndroidID__") < bid->cmonitorurl[k].size())
								{
									replaceAdmasterMonitor(hash_ctx, comrequest.device, bid->cmonitorurl[k], comrequest.id);
								}

								value_cm = json_new_string(bid->cmonitorurl[k].c_str());
								json_insert_child(ext_cm_entry, value_cm);
							}
						}

						json_insert_child(ext_cm, ext_cm_entry);
						json_insert_child(entry_ext, ext_cm);
					}

					if (bid->type)
					{
						switch(bid->ftype)
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
							if(bid->type == ADTYPE_FLASH)
								jsonInsertString(entry_ext, "type", "application/x-shockwave-flash");
							else
								jsonInsertString(entry_ext, "type", "unknown");
							break;
						}
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

	if(root != NULL)
		json_free_value(&root);

	return bresponse;
}

int letvResponse(IN uint8_t index,  IN uint64_t ctx, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata)
{
	int errcode = E_SUCCESS;

	if (recvdata == NULL || (recvdata->data == NULL) || (recvdata->length == 0))
	{
		errcode = E_BAD_REQUEST;
		writeLog(g_logid_local, LOGERROR, string("recvdata->data is null or recvdata->length is 0"));
		goto exit;
	}

	/* parse bid request,in the parseInmobiRequest doesn't parse user*/

	senddata = "Status: 204 OK\r\n\r\n";

exit:
	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);
		
	return errcode;
}
