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

// #include "../../common/setlog.h"
// #include "../../common/tcp.h"
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "adlefee_response.h"


//#define ADS_AMOUNT 20
extern map<string, vector<uint32_t> > app_cat_table;
//extern map<string, vector<uint32_t> > adv_cat_table_adxi;
//extern map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
extern map<string, uint16_t> dev_make_table;
extern int *numcount;
extern uint64_t geodb;
extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local, g_logid_response;
//static string g_ads_size[] = {"320x50", "640x100", "728x90", "1456x180", "468x60", "936x120",
//	"300x250", "600x500", "640x1136", "1136x640", "480x700", "700x480", "768x1024",
//	"1024x768", "560x750", "750x560", "240x290", "400x208", "1280x720", "780x400"};
//
//static uint32_t g_ads_enlarge[] = {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//static uint32_t g_ads_sizeratio[] = {5634, 6857, 7467, 7500, 8276, 12000, 12000, 13333, 13393,
//	14583, 17750, 17778, 19231, 19500, 64000, 64000, 78000, 78000, 80889, 80889};
//static uint32_t g_ads_sizeratio_upgrade_index[] = {8, 10, 14, 12, 16, 6, 7, 13, 15, 11, 9, 18, 17, 19, 0, 1, 4, 5, 2, 3};

extern MD5_CTX hash_ctx;


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

int parseAdlefeeRequest(char *data, COM_REQUEST &crequest)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;

	if (data == NULL)
		return E_BAD_REQUEST;


	json_parse_document(&root, data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseAdlefeeRequest root is NULL!");
		return E_BAD_REQUEST;
	}

	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		crequest.id = label->child->text;
		//request.id = label->child->text;
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

			cimp.type = IMP_BANNER;
			cimp.bidfloorcur = "CNY";
			if ((label = json_find_first_label(value_imp, "id")) != NULL)
				cimp.id = label->child->text;
			else
			{
				err = E_REQUEST_IMP_NO_ID;
				goto exit;
			}
			if ((label = json_find_first_label(value_imp, "bidfloor")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				cimp.bidfloor = atoi(label->child->text) / 10000.0;
			}

			if ((label = json_find_first_label(value_imp, "bidfloorcur")) != NULL)
				//request.bidfloorcur = label->child->text;
				cimp.bidfloorcur = label->child->text;

			if ((label = json_find_first_label(value_imp, "banner")) != NULL)
			{
				json_t *temp = label->child;
				if ((label = json_find_first_label(temp, "w")) != NULL
					&& label->child->type == JSON_NUMBER)
					cimp.banner.w = atoi(label->child->text);

				if ((label = json_find_first_label(temp, "h")) != NULL
					&& label->child->type == JSON_NUMBER)
					cimp.banner.h = atoi(label->child->text);

				if ((label = json_find_first_label(temp, "btype")) != NULL)
				{
					json_t *temp1 = label->child->child;
					//需要转换映射，待讨论
					while (temp1 != NULL)
					{
						uint16_t btype = atoi(temp1->text);
						if (btype == 1 || btype == 2)
						{
							cimp.banner.btype.insert(btype);
						}
						else
						{
							btype = 4;
							cimp.banner.btype.insert(btype);
						}
						temp1 = temp1->next;
					}
				}

				if ((label = json_find_first_label(temp, "battr")) != NULL)
				{
					json_t *temp1 = label->child->child;

					while (temp1 != NULL)
					{
						cimp.banner.battr.insert(atoi(temp1->text));
						temp1 = temp1->next;
					}
				}
			}

			if ((label = json_find_first_label(value_imp, "instl")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				cimp.ext.instl = atoi(label->child->text);
			}
			if ((label = json_find_first_label(value_imp, "secure")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				crequest.is_secure = atoi(label->child->text);
			}

			if ((label = json_find_first_label(value_imp, "ext")) != NULL)
			{
				json_t *temp = label->child;
				if ((label = json_find_first_label(temp, "splash")) != NULL
					&& label->child->type == JSON_NUMBER)
				{
					cimp.ext.splash = atoi(label->child->text);
					if (cimp.ext.splash)
					{
						//不支持开屏广告，直接返回nbr为0
						err = E_REQUEST_IMP_TYPE;
					}
				}
			}

			//request.imps.push_back(imp);
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
			crequest.app.id = crequest.app.bundle;
		}

		if ((label = json_find_first_label(temp, "storeurl")) != NULL)
			crequest.app.storeurl = label->child->text;
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *temp = label->child;
		string dpid1, dpid2, did;
		dpid1 = dpid2 = did = "";
		uint8_t dpidtype = DPID_UNKNOWN;

		crequest.device.devicetype = DEVICE_PHONE;
		if ((label = json_find_first_label(temp, "didsha1")) != NULL)
		{
			did = label->child->text;
			if (did != "")
			{
				crequest.device.dids.insert(make_pair(DID_IMEI_SHA1, did));
			}
		}
		if ((label = json_find_first_label(temp, "didmd5")) != NULL)
		{
			did = label->child->text;
			if (did != "")
			{
				crequest.device.dids.insert(make_pair(DID_IMEI_MD5, did));
			}
		}
		if ((label = json_find_first_label(temp, "dpidsha1")) != NULL)
		{
			dpid1 = label->child->text;
		}
		if ((label = json_find_first_label(temp, "dpidmd5")) != NULL)
		{
			dpid2 = label->child->text;
		}

		if ((label = json_find_first_label(temp, "macsha1")) != NULL)
		{
			string mac = label->child->text;
			if (mac != "")
			{
				crequest.device.dids.insert(make_pair(DID_MAC_SHA1, mac));
			}
		}
		if ((label = json_find_first_label(temp, "macmd5")) != NULL)
		{
			string mac = label->child->text;
			if (mac != "")
			{
				crequest.device.dids.insert(make_pair(DID_MAC_MD5, mac));
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
		if ((label = json_find_first_label(temp, "geo")) != NULL)
		{
			json_t * temp1 = label->child;
			if ((label = json_find_first_label(temp1, "lat")) != NULL)
				//double dlat = (atof)(label->child->text);
				crequest.device.geo.lat = double(atof(label->child->text));
			if ((label = json_find_first_label(temp1, "lon")) != NULL)
				//double lon = (atof)(label->child->text);
				crequest.device.geo.lon = double(atof(label->child->text));
			//cout <<"lon :" << crequest.device.geo.lon <<endl;
		}
		if ((label = json_find_first_label(temp, "carrier")) != NULL)
		{
			//request.device.carrier = label->child->text;
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
			crequest.device.os = 0;
			if (strcasecmp(label->child->text, "iPhone OS") == 0 || strcasecmp(label->child->text, "iOS") == 0)
			{
				crequest.device.os = 1;
				if (dpid1 != "")
				{
					crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, dpid1));
				}
				if (dpid2 != "")
				{
					crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, dpid2));
				}
			}
			else if (strcasecmp(label->child->text, "Android") == 0)
			{
				crequest.device.os = 2;
				if (dpid1 != "")
				{
					crequest.device.dpids.insert(make_pair(DPID_IDFA_SHA1, dpid1));
				}
				if (dpid2 != "")
				{
					crequest.device.dpids.insert(make_pair(DPID_IDFA_MD5, dpid2));
				}

			}
			else if (strcasecmp(label->child->text, "Windows") == 0)
			{
				crequest.device.os = 3;
			}

		}

		if (crequest.device.dids.empty() && crequest.device.dpids.empty())
		{
			err = E_REQUEST_DEVICE_ID;
		}

		if ((label = json_find_first_label(temp, "osv")) != NULL)
			crequest.device.osv = label->child->text;
		if ((label = json_find_first_label(temp, "connectiontype")) != NULL
			&& label->child->type == JSON_NUMBER)
			crequest.device.connectiontype = atoi(label->child->text);

		if ((label = json_find_first_label(temp, "ext")) != NULL)
		{
			json_t *temp1 = label->child;
			if ((label = json_find_first_label(temp1, "imei")) != NULL)
			{
				did = label->child->text;
				if (did != "")
				{
					crequest.device.dids.insert(make_pair(DID_IMEI, did));
				}
			}
			if ((label = json_find_first_label(temp1, "mac")) != NULL)
			{
				did = label->child->text;
				if (did != "")
				{
					crequest.device.dids.insert(make_pair(DID_MAC, did));
				}
			}
			if ((label = json_find_first_label(temp1, "openid")) != NULL)
			{
				did = label->child->text;
				if (did != "")
				{
					crequest.device.dids.insert(make_pair(DID_OTHER, did));
				}
			}
			if ((label = json_find_first_label(temp1, "anid")) != NULL)
			{
				dpid1 = label->child->text;
				if (dpid1 != "")
				{
					crequest.device.dpids.insert(make_pair(DPID_ANDROIDID, dpid1));
				}
			}

		}

	}

	crequest.cur.insert("CNY");

exit:
	if (root != NULL)
		json_free_value(&root);
	return err;
}

static void setAdlefeeJsonResponse(COM_REQUEST &request, COM_RESPONSE &cresponse, ADXTEMPLATE &adxtemp, string &data_out, int errcode)
{
	string trace_url_par = string("&") + get_trace_url_par(request, cresponse);
	char *text = NULL;
	json_t *root, *label_seatbid, *label_bid, *label_ext,
		*array_seatbid, *array_bid, *entry_seatbid, *entry_bid, *label_adm, *value_adm, *label_link, *value_link;
	uint32_t i, j;

	root = json_new_object();
	jsonInsertString(root, "id", request.id.c_str());

	if (errcode != E_SUCCESS)
	{
		jsonInsertInt(root, "nbr", 0);
		goto exit;
	}

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
					//replace_https_url(adxtemp,request.is_secure);

					entry_bid = json_new_object();
					jsonInsertString(entry_bid, "impid", bid.impid.c_str());
					float price = float(bid.price * 10000);

					jsonInsertFloat(entry_bid, "price", price, 2);
					jsonInsertString(entry_bid, "adid", bid.adid.c_str());

					if (adxtemp.nurl.size() > 0)
					{
						string nurl = adxtemp.nurl + trace_url_par;
						jsonInsertString(entry_bid, "nurl", nurl.c_str());
					}

					label_adm = json_new_string("adm");
					value_adm = json_new_object();
					string sourcurl = bid.sourceurl;
					if (sourcurl != "")
						jsonInsertString(value_adm, "img", sourcurl.c_str());
					jsonInsertInt(value_adm, "type", bid.type);

					label_link = json_new_string("link");
					value_link = json_new_object();
					//ctype需转换
					int ctype = CTYPE_UNKNOWN;
					switch (bid.ctype)
					{
					case CTYPE_OPEN_WEBPAGE:
						ctype = 1;
						break;
					case CTYPE_DOWNLOAD_APP:
						ctype = 2;
						break;
					case CTYPE_CALL:
						ctype = 3;
						break;
					case CTYPE_SENDSHORTMSG:
						ctype = 4;
						break;
					default:
						ctype = 0;
					}
					jsonInsertInt(value_link, "type", ctype);

					string curl = bid.curl;
					if (curl != "")
					{
						jsonInsertString(value_link, "lp", curl.c_str());
					}

					json_insert_child(label_link, value_link);
					json_insert_child(value_adm, label_link);

					json_insert_child(label_adm, value_adm);
					json_insert_child(entry_bid, label_adm);

					json_t *label_ext, *entry_ext;
					label_ext = json_new_string("ext");
					entry_ext = json_new_object();

					json_t *lable_imgT, *array_imgT, *value_imgT;
					lable_imgT = json_new_string("imgTrack");
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
							adxtemp.iurl[i] = adxtemp.iurl[i] + trace_url_par;
							value_imgT = json_new_string(adxtemp.iurl[i].c_str());
							json_insert_child(array_imgT, value_imgT);
						}
					}
					json_insert_child(lable_imgT, array_imgT);
					json_insert_child(entry_ext, lable_imgT);

					json_t *lable_cliT, *array_cliT, *value_cliT;
					lable_cliT = json_new_string("clkTrack");
					array_cliT = json_new_array();
					for (int i = 0; i < bid.cmonitorurl.size(); ++i)
					{
						//cout<<"bid.cmonitorurl[i] is: "<< bid.cmonitorurl[i]<<endl;
						value_cliT = json_new_string(bid.cmonitorurl[i].c_str());
						json_insert_child(array_cliT, value_cliT);
					}
					if (adxtemp.cturl.size() > 0)
					{
						for (int i = 0; i < adxtemp.cturl.size(); ++i)
						{
							adxtemp.cturl[i] = adxtemp.cturl[i] + trace_url_par;
							value_cliT = json_new_string(adxtemp.cturl[i].c_str());
							json_insert_child(array_cliT, value_cliT);
						}
					}
					json_insert_child(lable_cliT, array_cliT);
					json_insert_child(entry_ext, lable_cliT);

					json_insert_child(label_ext, entry_ext);
					json_insert_child(entry_bid, label_ext);

					if (bid.w > 0)
						jsonInsertInt(entry_bid, "w", bid.w);
					if (bid.h > 0)
						jsonInsertInt(entry_bid, "h", bid.h);

					json_t *label_cat, *entry_cat, *value_cat;
					label_cat = json_new_string("cat");
					entry_cat = json_new_array();
					set<uint32_t>::iterator p = bid.cat.begin();
					if (p != bid.cat.end())
					{
						char bufcat[64];
						sprintf(bufcat, "%d", *p);
						string cat = bufcat;
						writeLog(g_logid_local, LOGINFO, "bid=%s,cat=%s", cresponse.id.c_str(), cat.c_str());
						value_cat = json_new_string(cat.c_str());
						json_insert_child(entry_cat, value_cat);
					}
					json_insert_child(label_cat, entry_cat);
					json_insert_child(entry_bid, label_cat);
					/* */
					if (bid.cid.size() > 0)
						jsonInsertString(entry_bid, "cid", bid.cid.c_str());
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
	writeLog(g_logid_local, LOGINFO, "End setAdlefeeJsonResponse");
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

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata,
	OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
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

	err = parseAdlefeeRequest(recvdata->data, crequest);
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
		ferrcode = bid_filter_response(ctx, index, crequest, NULL, NULL, filter_cbidobject_callback, NULL, &cresponse, &adxtemp);
		err = ferrcode.errcode;
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		flow_bid.set_bid_flow(ferrcode);
		g_ser_log.send_bid_log(index, crequest, cresponse, err);
		setAdlefeeJsonResponse(crequest, cresponse, adxtemp, output, err);
	}

	senddata = "Content-Type: application/json\r\nx-lertb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

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
			writeLog(g_logid_local, LOGDEBUG, "response :" + output);
			numcount[index] = 0;
		}
	}
	//else if(fullreqrecord)
	//		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	//writeLog(g_logid_local, LOGDEBUG, "%s,spent time:%ld,err:0x%x,tid:0x%x,desc:%s", mrequest.id.c_str(), lasttime, err, datatransfer->threadlog,str_ferrcode.c_str());
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s, spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x", crequest.id.c_str(), lasttime, err);

	if (err != E_REDIS_CONNECT_INVALID)
		err = E_SUCCESS;
	/*back:
		if (recvdata->data != NULL)
		free(recvdata->data);
		if (recvdata != NULL)
		free(recvdata);*/

exit:
	flow_bid.set_err(err);
	return err;
}




