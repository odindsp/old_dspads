/*
 * adlefee_response.cpp
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

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <arpa/inet.h>


// #include "../../common/setlog.h"
// #include "../../common/tcp.h"
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "athome_response.h"


//#define ADS_AMOUNT 20
extern map<string, vector<uint32_t> > app_cat_table;
extern map<uint32_t, vector<uint32_t> > inputadcat;
extern map<uint32_t, uint32_t > outputadcat;
extern map<string, uint16_t> dev_make_table;
extern map<int32, string> advid_table;

extern int *numcount;
extern uint64_t geodb;
extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local, g_logid_response;

extern MD5_CTX hash_ctx;

#define base64EncodedLength(len)  (((len + 2) / 3) * 4)
#define base64DecodedLength(len)  (((len + 3) / 4) * 3)
#define VERSION_LENGTH 1
#define DEVICEID_LENGTH 1
#define CRC_LENGTH 4
#define KEY_LENGTH 16
typedef unsigned char uchar;

static int base64Decode(char* src, int srcLen, char* dst, int dstLen)
{
	BIO *bio, *b64;
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_new_mem_buf(src, srcLen);
	bio = BIO_push(b64, bio);
	int dstNewLen = BIO_read(bio, dst, dstLen);
	BIO_free_all(bio);
	return dstNewLen;
}

static bool decodeDeviceId(uchar* src, int len, uchar* didkey, uchar* realDeviceId)
{
	int deviceid_len = len - CRC_LENGTH;

	if (deviceid_len < 0 || len >= 64)
	{
		va_cout("Decode deviceId ERROR,  checksum error! ");
		return false;
	}

	uchar buf[64];

	int didkey_len = strlen((char*)didkey);
	memcpy(buf, didkey, didkey_len);
	uchar pad[KEY_LENGTH];
	MD5(buf, didkey_len, pad);

	uchar deviceId[64];
	uchar* pDeviceId = deviceId;
	uchar* pEncodeDeviceId = src;
	uchar* pXor = pad;
	uchar* pXorB = pXor;
	uchar* pXorE = pXorB + KEY_LENGTH;

	for (int i = 0; i != deviceid_len; ++i) {
		if (pXor == pXorE) {
			pXor = pXorB;
		}
		*pDeviceId++ = *pEncodeDeviceId++ ^ *pXor++;
	}

	memmove(realDeviceId, deviceId, deviceid_len);

	MD5(deviceId, deviceid_len, pad);
	if (0 != memcmp(pad, src + deviceid_len, CRC_LENGTH)) {
		//cerr << "checksum error!" <<endl;
		va_cout("Decode deviceId ERROR,  checksum error! ");
		//writeLog(g_logid_local,LOGERROR, "Decode deviceId ERROR,  checksum error! ");
		return false;
	}

	return true;
}

void decodeDeviceID(char *src, string &pid)
{

	int len = strlen(src);

	int origLen = base64DecodedLength(len);
	uchar* orig = new uchar[origLen];
	//uchar* didkey = (uchar*)("fbcc155a-a7fb-11e6-80f5-76304dec7eb7");
	if (orig != NULL)
	{
		uchar* didkey = (uchar*)("U8BR4DpL3V2wgTPy");

		origLen = base64Decode(src, len, (char*)orig, origLen);
		uchar deviceId[64] = { '\0' };

		if (decodeDeviceId(orig, origLen, didkey, deviceId))
		{
			pid = (char*)deviceId;
		}
		else
		{
			writeLog(g_logid_local, LOGDEBUG, "Decode deviceid failed, DeviceId is:%s", src);
		}

		delete[]orig;
	}
}

void init_app_object(APPOBJECT &app)
{

}

void init_device_object(DEVICEOBJECT &device)//
{
	device.deviceidtype = device.devicetype = device.sw = device.sh = device.orientation = device.os = 0;

	//device.did = device.dpid = device.mac = device.loc = device.carrier =  "";
}

void init_message_request(MESSAGEREQUEST &mrequest)//
{

	init_device_object(mrequest.device);

	mrequest.instl = mrequest.splash = 0;
}

void init_impression_object(IMPRESSIONOBJECT &imp)
{
	//imp.id = "";

	//imp.bidfloorcur = "CNY";

	imp.bidfloor = imp.instl = imp.splash = 0;

	//init_banner_object(imp.banner);
}

static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &ext)
{
	bool filter = true;

	if (ext.id == "")
		filter = false;

	return filter;
}

static bool filter_group_ext(IN const COM_REQUEST &crequest, IN const GROUPINFO_EXT &ext)
{
	if (ext.advid != "")
	{
		return true;
	}

	return false;
}
void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<uint32_t> > &adcat = *((map<uint32_t, vector<uint32_t> > *)data);
	uint32_t key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}

void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, uint32_t> &table = *((map<uint32_t, uint32_t> *)data);

	uint32_t key = 0;
	sscanf(key_start.c_str(), "%x", &key);//0x%x

	uint32_t i_val = 0;
	sscanf(val.c_str(), "%d", &i_val);
	table[key] = i_val;

}

void transfer_advid_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int32, string> &advid = *((map<int32, string> *)data);
	int32 key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	string value = val;
	advid.insert(pair<int32, string>(key, value));
}

void get_banner_info(string impInfo, uint32 &w, uint32 &h)
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
	return;
}

void get_native_info(string impInfo, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen)
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
					tLen = atoi(infoStr.c_str());
				}
				else if (tagStr == "des")
				{
					dLen = atoi(infoStr.c_str());
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

void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen)
{
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
			//cout<<"advtype is : "<< advtype <<endl;
		}
		if (advtype == 1)
		{
			get_banner_info(impInfo, w, h);
		}
		else
		{
			get_native_info(impInfo, w, h, tLen, dLen);
		}
	}
	return;
}

int parseAthomeRequest(char *data, COM_REQUEST &crequest, DATA_RESPONSE &dresponse)
{
	json_t *root = NULL, *label;
	uint32 w = 0;
	uint32 h = 0;
	uint32 tLen = 0;
	uint32 dLen = 0;
	int err = E_SUCCESS;

	if (data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseAthomeRequest root is NULL!");
		return E_BAD_REQUEST;
	}

	if ((label = json_find_first_label(root, "id")) != NULL && label->child->type == JSON_STRING)
	{
		crequest.id = label->child->text;
	}
	else
	{
		err = E_REQUEST_NO_BID;
		goto exit;
	}
	//汽车之家返回的url都为https
	crequest.is_secure = 0;

	if ((label = json_find_first_label(root, "version")) != NULL && label->child->type == JSON_STRING)
	{
		dresponse.version = label->child->text;
	}

	if ((label = json_find_first_label(root, "adSlot")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *value_imp = label->child->child;
		map<int32, string>::iterator iter;
		int32 advtype = 0;
		int tempId;
		string impInfo = "";

		if (value_imp != NULL && value_imp->type == JSON_OBJECT)
		{
			COM_IMPRESSIONOBJECT cimp;

			init_com_impression_object(&cimp);

			cimp.bidfloorcur = "CNY";
			if ((label = json_find_first_label(value_imp, "id")) != NULL && label->child->type == JSON_STRING)
				cimp.id = label->child->text;
			else
			{
				err = E_REQUEST_IMP_NO_ID;
				goto exit;
			}

			if ((label = json_find_first_label(value_imp, "slotid")) != NULL && label->child->type == JSON_STRING)
				dresponse.slotid = label->child->text;

			if ((label = json_find_first_label(value_imp, "min_cpm_price")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				cimp.bidfloor = atoi(label->child->text) / 100.0;
			}
			if ((label = json_find_first_label(value_imp, "deal_type")) != NULL && label->child->type == JSON_STRING)
			{
				string deal = label->child->text;
				if (deal != "RTB")
				{
					err = E_REQUEST_INVALID_BID_TYPE;
					goto exit;
				}
			}
			if ((label = json_find_first_label(value_imp, "slot_visibility")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int pos = atoi(label->child->text);

				if (pos == 1 || pos == 2)
				{
					cimp.adpos = pos;
				}
				else
					cimp.adpos = 3;
			}

			if ((label = json_find_first_label(value_imp, "excluded_ad_category")) != NULL && label->child->type == JSON_ARRAY)
			{
				json_t *temp1 = label->child->child;
				while (temp1 != NULL && temp1->type == JSON_NUMBER)
				{
					uint32_t bcat = atoi(temp1->text);

					map<uint32_t, vector<uint32_t> >::iterator cat = inputadcat.find(bcat);
					if (cat != inputadcat.end())
					{
						vector<uint32_t> &adcat = cat->second;
						for (uint32_t j = 0; j < adcat.size(); ++j)
							crequest.bcat.insert(adcat[j]);
					}
					temp1 = temp1->next;
				}
			}

			if ((label = json_find_first_label(value_imp, "banner")) != NULL && label->child->type == JSON_OBJECT)
			{
				json_t *temp = label->child;

				if ((label = json_find_first_label(temp, "templateId")) != NULL && label->child->type == JSON_ARRAY)
				{
					json_t *temp1 = label->child->child;
					crequest.device.devicetype = DEVICE_UNKNOWN;
					bool flag = false;
					while (temp1 != NULL && temp1->type == JSON_NUMBER)
					{
						tempId = atoi(temp1->text);

						if (tempId == 101 || tempId == 102 || tempId == 103 || tempId == 104)
						{
							break;
						}
						else
						{
							if (tempId != 0 && flag != true)
							{
								iter = advid_table.find(tempId);
								if (iter != advid_table.end())
								{
									flag = true;
									impInfo = iter->second;
									crequest.device.devicetype = DEVICE_PHONE;
									get_ImpInfo(impInfo, advtype, w, h, tLen, dLen);
								}
							}
							if (flag == true)
							{
								dresponse.templateld = tempId;
								break;
							}

						}
						temp1 = temp1->next;
					}
				}
				if (advtype == 1)
				{
					cimp.type = IMP_BANNER;
					cimp.banner.w = w;
					cimp.banner.h = h;
					dresponse.creative_type = 1002;
				}
				else
				{
					dresponse.creative_type = 1003;
					cimp.type = IMP_NATIVE;
					cimp.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
					COM_ASSET_REQUEST_OBJECT asset;

					if (w != 0 && h != 0)
					{
						asset.id = 1; // must have
						asset.required = 1;
						asset.type = NATIVE_ASSETTYPE_IMAGE;
						asset.img.type = ASSET_IMAGETYPE_MAIN;
						asset.img.w = w;
						asset.img.h = h;
						cimp.native.assets.push_back(asset);
					}
					if (tLen != 0)
					{
						asset.id = 2;
						asset.required = 1;
						asset.type = NATIVE_ASSETTYPE_TITLE;
						asset.title.len = (tLen / 2 * 3);
						cimp.native.assets.push_back(asset);
					}
					if (dLen != 0)
					{
						asset.id = 3; // must have
						asset.required = 1;
						asset.type = NATIVE_ASSETTYPE_DATA;
						asset.data.type = 2;
						asset.data.len = (dLen / 2 * 3);
						cimp.native.assets.push_back(asset);
					}
					if (tempId == 10002)
					{
						asset.required = 1;
						asset.type = NATIVE_ASSETTYPE_IMAGE;
						asset.img.type = ASSET_IMAGETYPE_ICON;
						asset.img.w = 148;
						asset.img.h = 112;
						cimp.native.assets.push_back(asset);
					}
					//add by youyang if tempId is 10026 put 3 assets        
					if (tempId == 10026)
					{
						for (int aindex = 0; aindex < 2; aindex++)
						{
							asset.required = 1;
							asset.type = NATIVE_ASSETTYPE_IMAGE;
							asset.img.type = ASSET_IMAGETYPE_MAIN;
							asset.img.w = w;
							asset.img.h = h;
							cimp.native.assets.push_back(asset);
						}
					}
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

	if ((label = json_find_first_label(root, "user")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "ip")) != NULL && label->child->type == JSON_STRING)
		{
			crequest.device.ip = label->child->text;

			if (crequest.device.ip.size() > 0)
			{
				int location = getRegionCode(geodb, crequest.device.ip);
				crequest.device.location = location;
			}
		}

		if ((label = json_find_first_label(temp, "user_agent")) != NULL && label->child->type == JSON_STRING)
		{
			crequest.device.ua = label->child->text;
		}

	}

	if ((label = json_find_first_label(root, "mobile")) != NULL && label->child->type == JSON_OBJECT)
	{

		json_t *temp = label->child;
		string dpid;
		//crequest.device.devicetype = DEVICE_PHONE;
		if ((label = json_find_first_label(temp, "is_app")) != NULL
			&& label->child->type == JSON_TRUE)
		{
			if ((label = json_find_first_label(temp, "pkgname")) != NULL && label->child->type == JSON_STRING)
			{
				crequest.app.id = label->child->text;
				crequest.app.bundle = crequest.app.id;
			}
		}

		if ((label = json_find_first_label(temp, "device")) != NULL && label->child->type == JSON_OBJECT)
		{
			json_t *temp1 = label->child;

			if ((label = json_find_first_label(temp1, "devicebrand")) != NULL && label->child->type == JSON_STRING)
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

			if ((label = json_find_first_label(temp1, "devicemodel")) != NULL && label->child->type == JSON_STRING)
			{
				crequest.device.model = label->child->text;
			}

			if ((label = json_find_first_label(temp1, "deviceid")) != NULL && label->child->type == JSON_STRING)
			{
				//需按照汽车之家提供的密钥解密后使用
				string encode_str = label->child->text;
				replaceMacro(encode_str, "\\", "");

				if (encode_str != "")
				{
					char* endid = new char[strlen(encode_str.c_str()) + 1];;
					strcpy(endid, encode_str.c_str());

					va_cout("before decode deviceId str is: %s", endid);
					decodeDeviceID(endid, dpid);
					//writeLog(g_logid_local, LOGDEBUG, "after decode deviceId str is : %s", dpid.c_str());

					if (endid)
					{
						delete[]endid;
						endid = NULL;
					}
				}
			}
			va_cout("after decode deviceId str is: %s", dpid.c_str());

			if ((label = json_find_first_label(temp1, "pm")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int os = atoi(label->child->text);
				if (os == 1)
				{
					crequest.device.os = OS_IOS;
					if (dpid != "")
					{
						crequest.device.dpids.insert(make_pair(DPID_IDFA, dpid));
					}
				}
				else if (os == 2)
				{
					crequest.device.os = OS_ANDROID;
					if (dpid != "")
					{
						crequest.device.dids.insert(make_pair(DID_IMEI, dpid));
					}
				}
				else
				{
					crequest.device.os = OS_UNKNOWN;
				}

			}
			if ((label = json_find_first_label(temp1, "os_version")) != NULL && label->child->type == JSON_STRING)
			{
				crequest.device.osv = label->child->text;
			}

			if ((label = json_find_first_label(temp1, "conn")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int conn = atoi(label->child->text);
				if (conn == 1)
				{
					crequest.device.connectiontype = CONNECTION_WIFI;
				}
				else if (conn == 2)
				{
					crequest.device.connectiontype = CONNECTION_CELLULAR_2G;
				}
				else if (conn == 3)
				{
					crequest.device.connectiontype = CONNECTION_CELLULAR_3G;
				}
				else if (conn == 4)
				{
					crequest.device.connectiontype = CONNECTION_CELLULAR_4G;
				}
				else
					crequest.device.connectiontype = CONNECTION_UNKNOWN;
			}
			if ((label = json_find_first_label(temp1, "networkid")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int carrier = CARRIER_UNKNOWN;
				carrier = atoi(label->child->text);
				if (carrier == 7012)
				{
					crequest.device.carrier = CARRIER_CHINAMOBILE;
				}
				else if (carrier == 70121)
				{
					crequest.device.carrier = CARRIER_CHINATELECOM;
				}
				else if (carrier == 70123)
				{
					crequest.device.carrier = CARRIER_CHINAUNICOM;
				}
				else
					crequest.device.carrier = CARRIER_UNKNOWN;

			}

			if ((label = json_find_first_label(temp1, "lat")) != NULL && label->child->type == JSON_NUMBER)
			{
				crequest.device.geo.lat = (double)((unsigned int)atoi(label->child->text) / 1000000.0);
			}
			if ((label = json_find_first_label(temp1, "lng")) != NULL && label->child->type == JSON_NUMBER)
			{
				crequest.device.geo.lon = (double)((unsigned int)atoi(label->child->text) / 1000000.0);
			}
		}
	}

	if (crequest.device.dids.empty() && crequest.device.dpids.empty())
	{
		err = E_REQUEST_DEVICE_ID;
	}

	crequest.cur.insert("CNY");

exit:
	if (root != NULL)
		json_free_value(&root);
	return err;
}

static void setAthomeJsonResponse(COM_REQUEST &request, COM_RESPONSE &cresponse, 
	ADXTEMPLATE &adxtemp, DATA_RESPONSE dresponse, string &data_out, int errcode)
{
	char *text = NULL;
	json_t *root, *label_seatbid, *label_ext, *label_con, *array_con, *object_con,
		*array_seatbid, *entry_seatbid, *entry_bid, *label_adm, *value_adm, *label_link, *value_link;
	uint32_t i, j;
	string trace_url_par = string("&") + get_trace_url_par(request, cresponse);
	root = json_new_object();
	jsonInsertString(root, "id", request.id.c_str());
	jsonInsertString(root, "version", dresponse.version.c_str());
	jsonInsertBool(root, "is_cm", false);

	if (!cresponse.seatbid.empty())
	{
		label_seatbid = json_new_string("ads");
		array_seatbid = json_new_array();

		for (i = 0; i < cresponse.seatbid.size(); ++i)
		{
			COM_SEATBIDOBJECT &seatbid = cresponse.seatbid[i];
			if (seatbid.bid.size() > 0)
			{

				for (j = 0; j < seatbid.bid.size(); ++j)
				{

					COM_BIDOBJECT &bid = seatbid.bid[j];
					//replace_https_url(adxtemp,request.is_secure);

					int templateld = dresponse.templateld;
					entry_bid = json_new_object();
					jsonInsertString(entry_bid, "id", bid.impid.c_str());
					jsonInsertString(entry_bid, "slotid", dresponse.slotid.c_str());

					int price = int(bid.price * 100);
					jsonInsertInt(entry_bid, "max_cpm_price", price);

					uint64_t mapid = atoi(bid.creative_ext.id.c_str());
					jsonInsertInt(entry_bid, "creative_id", mapid);

					//				    if(templateld == 10001)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910001);
					//				    }else if(templateld == 10002)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910002);
					//				    }else if(templateld == 10003)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910003);
					//				    }else if(templateld == 10004)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910004);
					//				    }else if(templateld == 10005)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910005);
					//				    }else if(templateld == 10006)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910006);
					//				    }else if(templateld == 10007)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910007);
					//				    }else if(templateld == 10008)
					//				    {
					//				    	jsonInsertInt(entry_bid, "creative_id", 9910008);
					//				    }

					uint32_t groupid = atoi(bid.groupinfo_ext.advid.c_str());
					jsonInsertInt(entry_bid, "advertiser_id", groupid);

					if (bid.w > 0)
						jsonInsertInt(entry_bid, "width", bid.w);
					if (bid.h > 0)
						jsonInsertInt(entry_bid, "height", bid.h);

					set<uint32_t>::iterator pcat = bid.cat.begin();
					for (pcat; pcat != bid.cat.end(); ++pcat)
					{
						uint32_t category = *pcat;
						uint32_t adcat = outputadcat[category];
						if (adcat != 0)
						{
							jsonInsertInt(entry_bid, "category", adcat);
						}
					}

					jsonInsertInt(entry_bid, "creative_type", dresponse.creative_type);
					jsonInsertInt(entry_bid, "templateId", dresponse.templateld);


					label_adm = json_new_string("adsnippet");
					value_adm = json_new_object();

					label_con = json_new_string("content");
					array_con = json_new_array();

					if (request.imp[0].type == IMP_BANNER)
					{
						object_con = json_new_object();
						string sourcurl = bid.sourceurl;
						if (sourcurl != "")
						{
							jsonInsertString(object_con, "src", sourcurl.c_str());
							jsonInsertString(object_con, "type", "img");
							json_insert_child(array_con, object_con);
						}
					}
					else if (request.imp[0].type == IMP_NATIVE)
					{
						int asset_pic_index = 0;
						for (int k = 0; k < bid.native.assets.size(); ++k)
						{
							object_con = json_new_object();

							COM_ASSET_RESPONSE_OBJECT asset = bid.native.assets[k];

							if (asset.type == NATIVE_ASSETTYPE_IMAGE)
							{
								COM_IMAGE_RESPONSE_OBJECT img = asset.img;
								jsonInsertString(object_con, "src", img.url.c_str());
								if (img.type == ASSET_IMAGETYPE_MAIN)
								{
									if (templateld == 10002)
										jsonInsertString(object_con, "type", "bimg");
									else if (templateld == 10026)
									{
										if (asset_pic_index == 0){
											jsonInsertString(object_con, "type", "img");
										}
										else if (asset_pic_index == 1){
											jsonInsertString(object_con, "type", "img2");
										}
										else if (asset_pic_index == 2){
											jsonInsertString(object_con, "type", "img3");
										}

										asset_pic_index += 1;
									}
									else
										jsonInsertString(object_con, "type", "img");

								}
								else if (img.type == ASSET_IMAGETYPE_ICON)
								{
									jsonInsertString(object_con, "type", "simg");
								}
								json_insert_child(array_con, object_con);

							}
							else if (asset.type == NATIVE_ASSETTYPE_TITLE)
							{
								jsonInsertString(object_con, "src", asset.title.text.c_str());
								jsonInsertString(object_con, "type", "text");
								json_insert_child(array_con, object_con);
							}
							else if (asset.type == NATIVE_ASSETTYPE_DATA)
							{
								jsonInsertString(object_con, "src", asset.data.value.c_str());
								jsonInsertString(object_con, "type", "stext");
								json_insert_child(array_con, object_con);
							}
						}
					}

					json_insert_child(label_con, array_con);
					json_insert_child(value_adm, label_con);


					string curl = bid.curl;

					if (adxtemp.cturl.size() > 0)
					{

						string curl_temp = curl;
						int en_curl_temp_len = 0;
						char *en_curl_temp = urlencode(curl_temp.c_str(), curl_temp.size(), &en_curl_temp_len);

						string adxtemp_str = adxtemp.cturl[0] + trace_url_par;
						string click_curl = adxtemp_str + "&curl=" + string(en_curl_temp, en_curl_temp_len);
						int en_click_curl_len = 0;
						char *en_click_curl = urlencode(click_curl.c_str(), click_curl.size(), &en_click_curl_len);
						curl = "%%CLICK_URL_UNESC%%&url=" + string(en_click_curl, en_click_curl_len);

						jsonInsertString(value_adm, "link", curl.c_str());

						free(en_curl_temp);
						free(en_click_curl);

					}

					json_t *lable_imgT, *array_imgT, *value_imgT;
					lable_imgT = json_new_string("pv");
					array_imgT = json_new_array();
					for (int i = 0; i < bid.imonitorurl.size(); ++i)
					{
						//cout<<"bid.imonitorurl[i] is: "<< bid.imonitorurl[i]<<endl;
						value_imgT = json_new_string(bid.imonitorurl[i].c_str());
						json_insert_child(array_imgT, value_imgT);
					}
					if (adxtemp.iurl.size() > 0)
					{
						for (int i = 0; i < adxtemp.iurl.size(); ++i)
						{
							string iurl = adxtemp.iurl[i] + trace_url_par;
							value_imgT = json_new_string(iurl.c_str());
							json_insert_child(array_imgT, value_imgT);
						}
					}
					json_insert_child(lable_imgT, array_imgT);
					json_insert_child(value_adm, lable_imgT);


					json_insert_child(label_adm, value_adm);
					json_insert_child(entry_bid, label_adm);
				}

			}
			json_insert_child(array_seatbid, entry_bid);
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
#ifdef DEBUG
	va_cout("response data is: %s", data_out.c_str());
#endif
	if (root != NULL)
		json_free_value(&root);

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "End setAutohomeJsonResponse");
#endif
	return;
}

void transform_request(MESSAGEREQUEST &mrequest, COM_REQUEST &crequest)
{

}

void transform_response(COM_RESPONSE &cresponse, MESSAGERESPONSE &mresponse)
{

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
	//RECVDATA *recvdata = arg;
	uint32_t sendlength = 0;
	//MESSAGEREQUEST mrequest;//adx msg request
	COM_REQUEST crequest;//common msg request
	string output = "";
	int ret;
	COM_RESPONSE cresponse;
	DATA_RESPONSE dresponse;
	string outputdata;//, senddata;
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

	err = parseAthomeRequest(recvdata->data, crequest, dresponse);
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
	}
	else if (err == E_SUCCESS)
	{
		init_com_message_response(&cresponse);
		init_ADXTEMPLATE(adxtemp);
		ferrcode = bid_filter_response(ctx, index, crequest, 
			NULL, filter_group_ext, filter_cbidobject_callback, filter_bid_ext, 
			&cresponse, &adxtemp);
		err = ferrcode.errcode;
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		flow_bid.set_bid_flow(ferrcode);
		setAthomeJsonResponse(crequest, cresponse, adxtemp, dresponse, output, err);
		g_ser_log.send_bid_log(index, crequest, cresponse, err);
	}

	senddata = "Content-Type: application/json\r\nx-lertb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

	if(err == E_SUCCESS)
	{
		numcount[index]++;
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", 
			cresponse.id.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(), 
			crequest.device.deviceid.c_str(), crequest.device.deviceidtype, 
			cresponse.seatbid[0].bid[0].price);

		if (numcount[index] % 1000 == 0)
		{
			writeLog(g_logid_local, LOGDEBUG, "response :" + output);
			numcount[index] = 0;
		}
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s, spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x", crequest.id.c_str(), lasttime, err);

	if (err != E_REDIS_CONNECT_INVALID)
		err = E_SUCCESS;

exit:
	flow_bid.set_err(err);
	return err;
}




