/*
 * adview_response.cpp
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

// #include "../../common/setlog.h"
// #include "../../common/tcp.h"
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "adview_response.h"

#define ICON "http://img.pxene.com/logo/pxlogo1.png"
#define ICON_s "https://img.pxene.com/logo/pxlogo1.png"

extern map<string, vector<uint32_t> > app_cat_table;
extern map<string, vector<uint32_t> > adv_cat_table_adxi;
extern map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
extern map<string, uint16_t> dev_make_table;
extern int *numcount;
extern uint64_t geodb;
extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local, g_logid_response;

extern MD5_CTX hash_ctx;


void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[key_start];

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

int parseAdviewRequest(char *data, COM_REQUEST &crequest)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;

	if (data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseAdviewRequest root is NULL!");
		return E_BAD_REQUEST;
	}

	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		crequest.id = label->child->text;
	}
	else
	{
		err = E_REQUEST_NO_BID;
		goto exit;
	}

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *value_imp = label->child->child;

		if (value_imp != NULL)
		{
			COM_IMPRESSIONOBJECT cimp;

			init_com_impression_object(&cimp);

			cimp.requestidlife = 3600;

			cimp.bidfloorcur = "CNY";

			string pid;

			if ((label = json_find_first_label(value_imp, "id")) != NULL)
				cimp.id = label->child->text;
			else
			{
				err = E_REQUEST_IMP_NO_ID;
				goto exit;
			}

			if ((label = json_find_first_label(value_imp, "tagid")) != NULL)
				pid = label->child->text;

			if ((label = json_find_first_label(value_imp, "banner")) != NULL)
			{
				cimp.type = IMP_BANNER;
				json_t *tmp = label->child;

				if ((label = json_find_first_label(tmp, "w")) != NULL
					&& label->child->type == JSON_NUMBER)
					cimp.banner.w = atoi(label->child->text);

				if ((label = json_find_first_label(tmp, "h")) != NULL
					&& label->child->type == JSON_NUMBER)
					cimp.banner.h = atoi(label->child->text);

				if ((label = json_find_first_label(tmp, "pos")) != NULL
					&& label->child->type == JSON_NUMBER)
				{
					int adpos = atoi(label->child->text);

					switch (adpos)
					{
					case 3:
					case 6:
					case 0x30:
					case 0x60:
						cimp.adpos = ADPOS_FIRST;
						break;
					case 1:
					case 4:
						cimp.adpos = ADPOS_HEADER;
						break;
					case 2:
					case 5:
						cimp.adpos = ADPOS_FOOTER;
						break;
					case 0x10:
					case 0x20:
					case 0x40:
					case 0x50:
						cimp.adpos = ADPOS_SIDEBAR;
						break;
					default:
						cimp.adpos = ADPOS_UNKNOWN;
						break;
					}
				}

				if ((label = json_find_first_label(tmp, "btype")) != NULL)
				{
					json_t *temp = label->child->child;
					//需要转换映射，待讨论
					while (temp != NULL)
					{
						cimp.banner.btype.insert(atoi(temp->text));
						temp = temp->next;
					}
				}

				if ((label = json_find_first_label(tmp, "battr")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						cimp.banner.battr.insert(atoi(temp->text));
						temp = temp->next;
					}
				}
			}
			else if ((label = json_find_first_label(value_imp, "native")) != NULL && label->child != NULL)
			{
				cimp.type = IMP_NATIVE;

				writeLog(g_logid_local, LOGDEBUG, "id=%s,pid=%s", crequest.id.c_str(), pid.c_str());

				json_t *tmp_native = label->child;
				if ((label = json_find_first_label(tmp_native, "request")) != NULL)
				{
					json_t *inner_native = label->child;
					/*if((label = json_find_first_label(inner_native, "ver")) != NULL)
					{
					if(strcmp(label->child->text, "1") != 0)
					err = E_INVALID_PARAM;
					}*/
					if ((label = json_find_first_label(inner_native, "layout")) != NULL && label->child->type == JSON_NUMBER)
					{
						//cimp.native.layout = atoi(label->child->text);
						//if(cimp.native.layout != NATIVE_LAYOUTTYPE_NEWSFEED && cimp.native.layout != NATIVE_LAYOUTTYPE_CONTENTSTREAM)
						//	err = E_INVALID_PARAM;
						cimp.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
					}
					if ((label = json_find_first_label(inner_native, "adunit")) != NULL && label->child->type == JSON_NUMBER)
					{

					}
					if ((label = json_find_first_label(inner_native, "plcmtcnt")) != NULL && label->child->type == JSON_NUMBER)
					{
						cimp.native.plcmtcnt = atoi(label->child->text);
					}
					if ((label = json_find_first_label(inner_native, "seq")) != NULL && label->child->type == JSON_NUMBER)
					{

					}
					if ((label = json_find_first_label(inner_native, "assets")) != NULL && label->child->type == JSON_ARRAY)
					{
						json_t *value_asset_detail = NULL;
						json_t *value_asset = label->child->child;
						while (value_asset != NULL)
						{
							COM_ASSET_REQUEST_OBJECT comassetobj;
							init_com_asset_request_object(comassetobj);
							if ((label = json_find_first_label(value_asset, "id")) != NULL && label->child->type == JSON_NUMBER)
							{
								comassetobj.id = atol(label->child->text);
							}
							if ((label = json_find_first_label(value_asset, "required")) != NULL && label->child->type == JSON_NUMBER)
							{
								comassetobj.required = atoi(label->child->text);
							}
							if ((label = json_find_first_label(value_asset, "title")) != NULL && label->child->type == JSON_OBJECT)
							{
								comassetobj.type = NATIVE_ASSETTYPE_TITLE;
								value_asset_detail = label->child;
								if ((label = json_find_first_label(value_asset_detail, "len")) != NULL && label->child->type == JSON_NUMBER)
								{
									int len = atoi(label->child->text);
									if (len == 0)
									{
										len = 30;
									}
									comassetobj.title.len = len * 3;
								}
							}
							if ((label = json_find_first_label(value_asset, "img")) != NULL && label->child->type == JSON_OBJECT)
							{
								comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
								value_asset_detail = label->child;
								//缺省为1
								comassetobj.img.type = ASSET_IMAGETYPE_ICON;
								if ((label = json_find_first_label(value_asset_detail, "type")) != NULL && label->child->type == JSON_NUMBER)
								{
									comassetobj.img.type = atoi(label->child->text);
								}
								if ((label = json_find_first_label(value_asset_detail, "w")) != NULL  && label->child->type == JSON_NUMBER)
									comassetobj.img.w = atoi(label->child->text);
								if ((label = json_find_first_label(value_asset_detail, "wmin")) != NULL  && label->child->type == JSON_NUMBER)
									comassetobj.img.wmin = atoi(label->child->text);
								if ((label = json_find_first_label(value_asset_detail, "h")) != NULL  && label->child->type == JSON_NUMBER)
									comassetobj.img.h = atoi(label->child->text);
								if ((label = json_find_first_label(value_asset_detail, "hmin")) != NULL  && label->child->type == JSON_NUMBER)
									comassetobj.img.hmin = atoi(label->child->text);
								comassetobj.img.mimes.insert(ADFTYPE_IMAGE_JPG);
								comassetobj.img.mimes.insert(ADFTYPE_IMAGE_GIF);
								//mimes为额外支持的图片格式
								if ((label = json_find_first_label(value_asset_detail, "mimes")) != NULL  && label->child->type == JSON_ARRAY)
								{
									//mime需由string转int存入后端com结构
									json_t *temp = label->child->child;
									while (temp != NULL)
									{
										if (temp->type == JSON_STRING && strlen(temp->text) > 0)
										{
											if (strcmp(temp->text, "image\\/png") == 0)
											{
												comassetobj.img.mimes.insert(ADFTYPE_IMAGE_PNG);
											}
										}
										temp = temp->next;
									}
								}
							}

							if ((label = json_find_first_label(value_asset, "video")) != NULL && label->child->type == JSON_OBJECT)
							{
								if (comassetobj.required == 1)
								{
									value_asset = value_asset->next;
									continue;
								}
							}

							if ((label = json_find_first_label(value_asset, "data")) != NULL && label->child->type == JSON_OBJECT)
							{
								comassetobj.type = NATIVE_ASSETTYPE_DATA;
								value_asset_detail = label->child;
								if ((label = json_find_first_label(value_asset_detail, "type")) != NULL  && label->child->type == JSON_NUMBER)
									comassetobj.data.type = atoi(label->child->text);
								if ((label = json_find_first_label(value_asset_detail, "len")) != NULL  && label->child->type == JSON_NUMBER)
								{
									int len = atoi(label->child->text);
									if (len == 0)
									{
										len = 20;
									}
									comassetobj.data.len = len * 3;
								}
							}

							cimp.native.assets.push_back(comassetobj);
							value_asset = value_asset->next;
						}
					}
				}
				/*if((label = json_find_first_label(tmp_native, "ver")) != NULL)
				{
				if(strcmp(label->child->text, "1.0") != 0)
				{
				err = E_INVALID_PARAM;
				}
				}*/
			}

			if ((label = json_find_first_label(value_imp, "bidfloor")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				cimp.bidfloor = atoi(label->child->text) / 10000.0;
			}

			if ((label = json_find_first_label(value_imp, "bidfloorcur")) != NULL)
			{
				if (strcmp(label->child->text, "USD") == 0)
					cimp.bidfloorcur = "USD";
			}

			if ((label = json_find_first_label(value_imp, "instl")) != NULL && label->child->type == JSON_NUMBER)
			{
				int instl = atoi(label->child->text);
				switch (instl)
				{
				case 0:
				case 1:
				{
					cimp.ext.instl = instl;
					break;
				}
				case 4:
				{
					cimp.ext.splash = 1;
					break;
				}
				//case 6:
				default:
					break;
				}

			}
			crequest.imp.push_back(cimp);
		}
	}
	else
	{
		err = E_REQUEST_IMP_SIZE_ZERO;
		goto exit;
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "id")) != NULL)
		{
			crequest.app.id = label->child->text;
		}
		if ((label = json_find_first_label(temp, "name")) != NULL)
			crequest.app.name = label->child->text;
		if ((label = json_find_first_label(temp, "cat")) != NULL)
		{
			//待映射
			json_t *temp = label->child->child;
			while (temp != NULL)
			{
				vector<uint32_t> &v_cat = app_cat_table[temp->text];
				for (uint32_t i = 0; i < v_cat.size(); ++i)
					crequest.app.cat.insert(v_cat[i]);

				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(temp, "bundle")) != NULL)
		{
			crequest.app.bundle = label->child->text;
		}

		if ((label = json_find_first_label(temp, "storeurl")) != NULL)
			crequest.app.storeurl = label->child->text;
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *temp = label->child;
		string dpidsha1, dpidmd5;
		uint8_t dpidtype_sha1 = DPID_UNKNOWN;
		uint8_t dpidtype_md5 = DPID_UNKNOWN;

		if ((label = json_find_first_label(temp, "didsha1")) != NULL)
		{
			string didsha1 = label->child->text;
			if (didsha1 != "")
			{
				transform(didsha1.begin(), didsha1.end(), didsha1.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_IMEI_SHA1, didsha1));
			}
		}

		if ((label = json_find_first_label(temp, "didmd5")) != NULL)
		{
			string didmd5 = label->child->text;
			if (didmd5 != "")
			{
				transform(didmd5.begin(), didmd5.end(), didmd5.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_IMEI_MD5, didmd5));
			}
		}

		if ((label = json_find_first_label(temp, "dpidsha1")) != NULL)
		{
			dpidsha1 = label->child->text;
		}

		if ((label = json_find_first_label(temp, "dpidmd5")) != NULL)
		{
			dpidmd5 = label->child->text;
		}

		if ((label = json_find_first_label(temp, "macsha1")) != NULL)
		{
			string macsha1 = label->child->text;
			if (macsha1 != "")
			{
				transform(macsha1.begin(), macsha1.end(), macsha1.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_MAC_SHA1, macsha1));
			}
		}

		if ((label = json_find_first_label(temp, "macmd5")) != NULL)
		{
			string macmd5 = label->child->text;
			if (macmd5 != "")
			{
				transform(macmd5.begin(), macmd5.end(), macmd5.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_MAC_MD5, macmd5));
			}
		}

		if ((label = json_find_first_label(temp, "ua")) != NULL)
			crequest.device.ua = label->child->text;
		if ((label = json_find_first_label(temp, "ip")) != NULL)
		{
			crequest.device.ip = label->child->text;
			if (crequest.device.ip.size() > 0)
			{
				int location = getRegionCode(geodb, crequest.device.ip);
				crequest.device.location = location;
			}
		}

		if ((label = json_find_first_label(temp, "carrier")) != NULL)
		{
			int carrier = CARRIER_UNKNOWN;
			carrier = atoi(label->child->text);
			if (carrier == 46000 || carrier == 46002 || carrier == 46007 || carrier == 46008 || carrier == 46020)
				crequest.device.carrier = CARRIER_CHINAMOBILE;
			else if (carrier == 46001 || carrier == 46006 || carrier == 46009)
				crequest.device.carrier = CARRIER_CHINAUNICOM;
			else if (carrier == 46003 || carrier == 46005 || carrier == 46011)
				crequest.device.carrier = CARRIER_CHINATELECOM;

		}

		if ((label = json_find_first_label(temp, "make")) != NULL)
		{
			string s_make = label->child->text;
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
		if ((label = json_find_first_label(temp, "model")) != NULL)
			crequest.device.model = label->child->text;

		if ((label = json_find_first_label(temp, "os")) != NULL)
		{
			//将os转为int
			if (strcasecmp(label->child->text, "iOS") == 0)
			{
				crequest.device.os = 1;
				crequest.is_secure = 0;		//adview暂时取消操作系统区分请求是否安全
				dpidtype_sha1 = DPID_IDFA_SHA1;
				dpidtype_md5 = DPID_IDFA_MD5;
				if ((label = json_find_first_label(temp, "idfa")) != NULL)
				{
					string idfa = label->child->text;
					if (idfa != "")
					{
						crequest.device.dpids.insert(make_pair(DPID_IDFA, idfa));
					}
				}
			}
			else if (strcasecmp(label->child->text, "Android") == 0)
			{
				crequest.device.os = 2;
				crequest.is_secure = 0;
				dpidtype_sha1 = DPID_ANDROIDID_SHA1;
				dpidtype_md5 = DPID_ANDROIDID_MD5;
			}
			else if (strcasecmp(label->child->text, "Windows") == 0)
			{
				crequest.device.os = 3;
				dpidtype_sha1 = DPID_OTHER;
			}

			if (dpidsha1.size() > 0)
				crequest.device.dpids.insert(make_pair(dpidtype_sha1, dpidsha1));
			if (dpidmd5 != "")
				crequest.device.dpids.insert(make_pair(dpidtype_md5, dpidmd5));
		}

		if (crequest.device.dids.empty() && crequest.device.dpids.empty())
		{
			err = E_REQUEST_DEVICE_ID;
		}

		if ((label = json_find_first_label(temp, "osv")) != NULL)
			crequest.device.osv = label->child->text;
		if ((label = json_find_first_label(temp, "connectiontype")) != NULL
			&& label->child->type == JSON_NUMBER)
		{
			int connectiontype = atoi(label->child->text);
			switch (connectiontype)
			{
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				connectiontype--;
				break;
			default:
				connectiontype = CONNECTION_UNKNOWN;
			}
			crequest.device.connectiontype = connectiontype;
		}
		if ((label = json_find_first_label(temp, "devicetype")) != NULL
			&& label->child->type == JSON_NUMBER)
		{
			//devicetype需映射
			int devicetype = atoi(label->child->text);
			switch (devicetype)
			{
			case 1:
			case 2:
			case 4:
				crequest.device.devicetype = DEVICE_PHONE;
				break;
			case 3:
			case 5:
				crequest.device.devicetype = DEVICE_TABLET;
				break;
			default:
				crequest.device.devicetype = DEVICE_UNKNOWN;
				break;
			}
		}

		if ((label = json_find_first_label(temp, "geo")) != NULL)
		{
			json_t *tmp = label->child;
			if ((label = json_find_first_label(tmp, "lat")) != NULL && label->child->type == JSON_NUMBER)
				crequest.device.geo.lat = atof(label->child->text);
			if ((label = json_find_first_label(tmp, "lon")) != NULL && label->child->type == JSON_NUMBER)
				crequest.device.geo.lon = atof(label->child->text);
		}
	}
	else
	{
		err = E_REQUEST_DEVICE;
	}

	if ((label = json_find_first_label(root, "user")) != NULL)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "id")) != NULL)
		{
			crequest.user.id = label->child->text;
		}
		if ((label = json_find_first_label(temp, "keywords")) != NULL)
			crequest.user.keywords = label->child->text;


		if ((label = json_find_first_label(temp, "gender")) != NULL)
		{
			string gender = label->child->text;
			if (gender == "M")
				crequest.user.gender = GENDER_MALE;
			else if (gender == "F")
				crequest.user.gender = GENDER_FEMALE;
			else
				crequest.user.gender = GENDER_UNKNOWN;
		}

		if ((label = json_find_first_label(temp, "yob")) != NULL && label->child->type == JSON_NUMBER)
			crequest.user.yob = atoi(label->child->text);

		if ((label = json_find_first_label(temp, "geo")) != NULL)
		{
			json_t *tmp = label->child;
			if ((label = json_find_first_label(tmp, "lat")) != NULL && label->child->type == JSON_NUMBER)
				crequest.user.geo.lat = atof(label->child->text);
			if ((label = json_find_first_label(tmp, "lon")) != NULL && label->child->type == JSON_NUMBER)
				crequest.user.geo.lon = atof(label->child->text);
		}

	}

	if ((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
	{
		int at = atoi(label->child->text);//2 – 优先购买(不参与竞价) 
		
		if(at == 1 || at == 0)
		{
			crequest.at = BID_RTB;
		}
		if (at == 2)
		{
			crequest.at = BID_PMP;
		}
	}

	if ((label = json_find_first_label(root, "bcat")) != NULL &&label->child != NULL)
	{
		//需通过映射,传递给后端
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			vector<uint32_t> &v_cat = adv_cat_table_adxi[temp->text];
			for (uint32_t i = 0; i < v_cat.size(); ++i)
				crequest.bcat.insert(v_cat[i]);

			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL &&label->child != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			crequest.badv.insert(temp->text);
			temp = temp->next;
		}
	}
	if ((label = json_find_first_label(root, "cur")) != NULL &&label->child != NULL)
	{
		//CPM,CPC
	}
	crequest.cur.insert("CNY");

	if (crequest.app.id == "e1e7155b319e1fce820ddf1b81aaf7e4" || crequest.app.id == "373337c09effeefbb956547f33c74da4")
	{
		crequest.device.make = 255;
		crequest.device.makestr = "unknown";
	}

exit:
	if (root != NULL)
		json_free_value(&root);
	return err;
}

static void setAdviewJsonResponse(COM_REQUEST &request, COM_RESPONSE &cresponse, ADXTEMPLATE &adxtemp, string &data_out, int errcode)
{
	char *text = NULL;
	json_t *root, *label_seatbid, *label_bid, *label_ext,
		*array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;
	string trace_url_par = string("&") + get_trace_url_par(request, cresponse);

	root = json_new_object();
	jsonInsertString(root, "id", request.id.c_str());

	if (errcode != E_SUCCESS)
	{
		jsonInsertInt(root, "nbr", 0);
		goto exit;
	}

	if (cresponse.bidid.size() > 0)
		jsonInsertString(root, "bidid", cresponse.bidid.c_str());

	if (!cresponse.seatbid.empty())
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();
		for (i = 0; i < cresponse.seatbid.size(); ++i)
		{
			COM_SEATBIDOBJECT &seatbid = cresponse.seatbid[i];
			entry_seatbid = json_new_object();
			if (seatbid.bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();
				for (j = 0; j < seatbid.bid.size(); ++j)
				{
					COM_BIDOBJECT &bid = seatbid.bid[j];
					entry_bid = json_new_object();
					jsonInsertString(entry_bid, "impid", bid.impid.c_str());
					jsonInsertInt(entry_bid, "price", bid.price * 10000);
					jsonInsertInt(entry_bid, "paymode", 1);

					//replace_https_url(adxtemp,request.is_secure);

					//ctype需转换
					int ctype = 0;
					switch (bid.ctype)
					{
					case CTYPE_OPEN_WEBPAGE:
					{
						ctype = 1;
						break;
					}
					case CTYPE_DOWNLOAD_APP:
					case CTYPE_DOWNLOAD_APP_FROM_APPSTORE:
					{
						ctype = 2;
						break;
					}
					case CTYPE_SENDSHORTMSG:
					{
						ctype = 8;
						break;
					}
					case CTYPE_CALL:
					{
						ctype = 32;
						break;
					}
					default:
						ctype = 0;
					}
					jsonInsertInt(entry_bid, "adct", ctype);
					//广告主审核返回的id
					//jsonInsertString(entry_bid, "adid", bid->adid.c_str());

					//admt,adm,native
					string curl = bid.curl;

					int k = 0;
					//根据redirect判断是否需要拼接desturl
					if (bid.redirect)
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

					int type = 0;
					switch (bid.type)
					{
					case ADTYPE_TEXT_LINK:
					{
						type = 3;
						break;
					}
					case ADTYPE_IMAGE:
					{
						type = 1;
						break;
					}
					case ADTYPE_GIF_IMAGE:
					{
						type = 2;
						break;
					}
					case ADTYPE_HTML5:
					{
						type = bid.type;
						if (adxtemp.adms.size() > 0)
						{
							string adm = "";
							//根据os和type查找对应的adm
							for (int count = 0; count < adxtemp.adms.size(); ++count)
							{
								if (adxtemp.adms[count].os == request.device.os && adxtemp.adms[count].type == bid.type)
								{
									adm = adxtemp.adms[count].adm;
									break;
								}
							}

							if (adm.size() > 0)
							{
								replaceMacro(adm, "#SOURCEURL#", bid.sourceurl.c_str());
								//IURL需要替换
								string iurl;
								if (adxtemp.iurl.size() > 0)
								{
									iurl = adxtemp.iurl[0] + trace_url_par;
								}

								replaceMacro(adm, "#IURL#", iurl.c_str());
								jsonInsertString(entry_bid, "adm", adm.c_str());
							}

						}
						break;
					}
					case ADTYPE_MRAID_V2:
					case ADTYPE_VIDEO:
					case ADTYPE_FLASH:
					{
						type = bid.type;
						break;
					}
					case ADTYPE_FEEDS:
					{
						type = 8;
						//insert native
						json_t *native_label, *native_object, *label_ver, *label_link, *object_link;
						string nativecurl;

						native_label = json_new_string("native");
						native_object = json_new_object();
						object_link = json_new_object();
						label_link = json_new_string("link");
						if (bid.redirect == 0)
						{
							if (adxtemp.cturl.size() > 0)
							{
								int lenout;
								string strtemp = curl;
								char *enurl = urlencode(strtemp.c_str(), strtemp.size(), &lenout);
								nativecurl = adxtemp.cturl[0] + trace_url_par;
								++k;
								nativecurl += "&curl=" + string(enurl, lenout);
								free(enurl);
							}
						}
						else
							nativecurl = curl;
						jsonInsertString(object_link, "url", nativecurl.c_str());
						json_insert_child(label_link, object_link);
						json_insert_child(native_object, label_link);
						jsonInsertString(native_object, "ver", "1");

						if (bid.native.assets.size() > 0)
						{
							json_t *assetObj, *assetArray, *assetLabel;
							assetLabel = json_new_string("assets");
							assetArray = json_new_array();

							for (int i = 0; i < bid.native.assets.size(); ++i)
							{
								COM_ASSET_RESPONSE_OBJECT &comasetrepobj = bid.native.assets[i];

								if (comasetrepobj.required == 1)
								{
									json_t *assetLabel = NULL;
									json_t *innerobj;
									assetObj = json_new_object();
									innerobj = json_new_object();
									jsonInsertInt(assetObj, "id", comasetrepobj.id);
									switch (comasetrepobj.type)
									{
									case NATIVE_ASSETTYPE_TITLE:
									{
										assetLabel = json_new_string("title");
										jsonInsertString(innerobj, "text", comasetrepobj.title.text.c_str());
										json_insert_child(assetLabel, innerobj);
										json_insert_child(assetObj, assetLabel);
										break;
									}
									case NATIVE_ASSETTYPE_IMAGE:
									{
										assetLabel = json_new_string("img");
										jsonInsertString(innerobj, "url", comasetrepobj.img.url.c_str());
										jsonInsertInt(innerobj, "w", comasetrepobj.img.w);
										jsonInsertInt(innerobj, "h", comasetrepobj.img.h);
										json_insert_child(assetLabel, innerobj);
										json_insert_child(assetObj, assetLabel);
										break;
									}
									case NATIVE_ASSETTYPE_DATA:
									{
										assetLabel = json_new_string("data");
										if (comasetrepobj.data.label.size() > 0)
											jsonInsertString(innerobj, "label", comasetrepobj.data.label.c_str());
										jsonInsertString(innerobj, "value", comasetrepobj.data.value.c_str());
										json_insert_child(assetLabel, innerobj);
										json_insert_child(assetObj, assetLabel);
										break;
									}
									}
									json_insert_child(assetArray, assetObj);
								}
							}
							json_insert_child(assetLabel, assetArray);
							json_insert_child(native_object, assetLabel);

							json_insert_child(native_label, native_object);
							json_insert_child(entry_bid, native_label);
						}
						break;
					}
					}
					jsonInsertInt(entry_bid, "admt", type);
					if (bid.type != ADTYPE_FEEDS)
					{
						jsonInsertString(entry_bid, "adi", bid.sourceurl.c_str());
						//adt,ads
						jsonInsertInt(entry_bid, "adw", bid.w);
						jsonInsertInt(entry_bid, "adh", bid.h);
					}
					//adtm,ade,adurl

					if (bid.adomain.size() > 0)
						jsonInsertString(entry_bid, "adomain", bid.adomain.c_str());
					//废弃nurl
					if (!adxtemp.nurl.empty() && adxtemp.nurl.size() > 0)
					{
						string wurl = adxtemp.nurl + trace_url_par;
						jsonInsertString(entry_bid, "wurl", wurl.c_str());
					}

					if (bid.type != ADTYPE_HTML5)
					{//展现检测
						string iurl = "";
						json_t *label_iurl, *object_iurl, *array_iurl, *value_iurl, *label_inner;

						label_iurl = json_new_string("nurl");
						object_iurl = json_new_object();
						label_inner = json_new_string("0");
						array_iurl = json_new_array();

						if (adxtemp.iurl.size() > 0)
						{
							iurl = adxtemp.iurl[0] + trace_url_par;
							value_iurl = json_new_string(iurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}

						for (int k = 0; k < bid.imonitorurl.size(); ++k)
						{
							iurl = bid.imonitorurl[k];

							value_iurl = json_new_string(iurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}
						json_insert_child(label_inner, array_iurl);
						json_insert_child(object_iurl, label_inner);
						json_insert_child(label_iurl, object_iurl);
						json_insert_child(entry_bid, label_iurl);
					}

					if (bid.type != ADTYPE_FEEDS)
						jsonInsertString(entry_bid, "adurl", curl.c_str());

					if (1)
					{
						json_t *label_cturl, *array_cturl, *value_cturl;

						label_cturl = json_new_string("curl");
						array_cturl = json_new_array();

						for (; k < adxtemp.cturl.size(); ++k)
						{
							string cturl = adxtemp.cturl[k] + trace_url_par;
							cturl += "&url=";
							value_cturl = json_new_string(cturl.c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						for (k = 0; k < bid.cmonitorurl.size(); ++k)
						{
							value_cturl = json_new_string(bid.cmonitorurl[k].c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						json_insert_child(label_cturl, array_cturl);
						json_insert_child(entry_bid, label_cturl);
					}

					//创意审核返回的id
					/*if (bid->cid.size() > 0)
						jsonInsertString(entry_bid, "cid", bid->cid.c_str());*/
					if (bid.crid.size() > 0)
						jsonInsertString(entry_bid, "crid", bid.crid.c_str());

					if (bid.attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						set<uint8_t>::iterator it; //定义前向迭代器 遍历集合中的所有元素  

						for (it = bid.attr.begin(); it != bid.attr.end(); ++it)
						{
							//va_cout("attr is %d", *it);  
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							value_attr = json_new_string(buf);
							json_insert_child(array_attr, value_attr);
						}
						json_insert_child(label_attr, array_attr);
						json_insert_child(entry_bid, label_attr);
					}
					//if (bid.adomain.size() > 0)
					//	jsonInsertString(entry_bid, "adomain", bid.adomain.c_str());

					{
						GROUPINFO_EXT &ext_gr = bid.groupinfo_ext;
						if (ext_gr.advid.size() > 0)//广告主审核返回的id
							jsonInsertString(entry_bid, "adid", ext_gr.advid.c_str());

						CREATIVE_EXT &ext = bid.creative_ext;
						if (ext.id.size() > 0)//创意审核返回的id
							jsonInsertString(entry_bid, "cid", ext.id.c_str());

						if (request.imp[0].ext.splash != 0 && ext.ext.size() > 0)
						{
							json_t *tmproot = NULL;
							json_t *tmplabel;

							json_parse_document(&tmproot, ext.ext.c_str());
							if (tmproot != NULL)
							{
								if (tmplabel = json_find_first_label(tmproot, "ade"))
									if (strlen(tmplabel->child->text) != 0)
										jsonInsertString(entry_bid, "ade", tmplabel->child->text);
								if ((tmplabel = json_find_first_label(tmproot, "adtm")) != NULL && tmplabel->child->type == JSON_NUMBER)
									jsonInsertInt(entry_bid, "adtm", atoi(tmplabel->child->text));
								json_free_value(&tmproot);
							}
						}
					}

					json_insert_child(array_bid, entry_bid);
				}
				json_insert_child(label_bid, array_bid);
				json_insert_child(entry_seatbid, label_bid);
			}
			json_insert_child(array_seatbid, entry_seatbid);
		}
		json_insert_child(label_seatbid, array_seatbid);
		json_insert_child(root, label_seatbid);
	}
exit:
	json_tree_to_string(root, &text);
	if (text != NULL)
	{
		data_out = text;
		free(text);
		text = NULL;
	}

	if (root != NULL)
		json_free_value(&root);

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "End setAdviewJsonResponse");
#endif
	return;
}

static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &ext)
{
	bool filter = true;

	if (ext.id == "")
		filter = false;

	return filter;
}

static bool callback_process_crequest(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = adxtemplate.ratio;

	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
		return false;

	for (int i = 0; i < crequest.imp.size(); ++i)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];

		double bidfloor_adx = cimp.bidfloor;

		if (cimp.bidfloorcur == "USD")
		{
			cimp.bidfloor /= ratio;
		}
		else if (cimp.bidfloorcur != "CNY")
		{
			return false;
		}

		cflog(g_logid_local, LOGINFO, "process_crequest_callback, bidfloor_adx[%s]:%f -> bidfloor[CNY]:%f",
			cimp.bidfloorcur.c_str(), bidfloor_adx, cimp.bidfloor);
	}

	return true;
}

static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	//	double ratio = adxtemplate.ratio;
	if (crequest.at == BID_PMP)
		return true;

	if ((ratio > -0.000001) && (ratio < 0.000001))
	{
		cflog(g_logid_local, LOGERROR, "callback_check_price failed! ratio:%f", ratio);

		return false;
	}

	for (int i = 0; i < crequest.imp.size(); ++i)
	{
		double price_gap_usd = (price - crequest.imp[i].bidfloor) * ratio;

		if (price_gap_usd < 0.01)//1 USD cent
		{
			cflog(g_logid_local, LOGERROR, "callback_check_price failed! bidfloor[cny]:%f, cbid.price[cny]:%f, ratio:%f, price_gap_usd[usd]:%f",
				crequest.imp[i].bidfloor, price, ratio, price_gap_usd);

			return false;
		}
		else
		{
			cflog(g_logid_local, LOGINFO, "callback_check_price success! bidfloor[cny]:%f, cbid.price[cny]:%f, ratio:%f, price_gap_usd[usd]:%f",
				crequest.imp[i].bidfloor, price, ratio, price_gap_usd);
		}
	}

	return true;
}

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int err = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	uint32_t sendlength = 0;
	COM_REQUEST crequest;
	string output = "";
	int ret;
	COM_RESPONSE cresponse;
	string outputdata;
	ADXTEMPLATE adxtemp;
	struct timespec ts1, ts2;
	long lasttime;
	string str_ferrcode = "";

	if (recvdata == NULL)
	{
		err = E_BID_PROGRAM;
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		err = E_BID_PROGRAM;
		goto exit;
	}
	getTime(&ts1);
	init_com_message_request(&crequest);

	err = parseAdviewRequest(recvdata->data, crequest);
	if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);
		goto exit;
	}
	flow_bid.set_req(crequest);

	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s, getRegionCode ip:%s location:%d failed!", 
			crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		err = E_REQUEST_DEVICE_IP;
		goto exit;
	}

	init_com_message_response(&cresponse);
	ferrcode = bid_filter_response(ctx, index, crequest, 
		callback_process_crequest, NULL, callback_check_price, filter_bid_ext, 
		&cresponse, &adxtemp);

	err = ferrcode.errcode;
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	flow_bid.set_bid_flow(ferrcode);
	setAdviewJsonResponse(crequest, cresponse, adxtemp, output, err);
	g_ser_log.send_bid_log(index, crequest, cresponse, err);

	if (err == E_SUCCESS)
	{
		senddata = string("Content-Type: application/json\r\nx-adviewrtb-version: 2.3\r\nContent-Length: ")
			+ intToString(output.size()) + "\r\n\r\n" + output;
	}

exit:
	if (err == E_SUCCESS)
	{
		numcount[index]++;
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", 
			cresponse.id.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(), crequest.device.deviceid.c_str(), 
			crequest.device.deviceidtype, cresponse.seatbid[0].bid[0].price);
		if (numcount[index] % 1000 == 0)
		{
			writeLog(g_logid_local, LOGDEBUG, string("response :") + output);
			numcount[index] = 0;
		}
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x", crequest.id.c_str(), lasttime, err);

	flow_bid.set_err(err);
	return err;
}
