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
#include "homeTV_response.h"

extern map<string, uint16_t> dev_make_table;
extern int *numcount;
extern uint64_t geodb;
extern uint64_t g_logid, g_logid_local, g_logid_response;

extern MD5_CTX hash_ctx;
extern map<int, int> adstyle_to_pos;
#define FREE(p) {if(p) {free(p);p = NULL;}}

int parseHomeTVRequest(IN char *requestparam, IN char *reqaddr, OUT COM_REQUEST &crequest)
{
	int err = E_SUCCESS;
	char *id = NULL;
	char *adid = NULL;
	char *adtype_s = NULL;
	char *adstyle_s = NULL;
	int adtype = 0;
	//int adstyle = 0;
	char *net_s = NULL;
	char *carrier_s = NULL;
	char *os_s = NULL;
	char *osv = NULL;
	char *make_s = NULL;
	//char *imei = NULL;
	char *ua = NULL;
	char *wma = NULL;
	char *andid = NULL;
	char *idfa = NULL;
	char *lat = NULL;
	char *lon = NULL;
	char *scrro_s = NULL;
	char *pos_s = NULL;
	char *devicetype_s = NULL;
	char *istest = NULL;
	int devicetype = 0;
	int scrro = 0;
	int adstyle = 0;
	COM_IMPRESSIONOBJECT cimp;
	int w;
	int h;

	init_com_impression_object(&cimp);
	cimp.requestidlife = 7200;

	crequest.id = "test";

	id = getParamvalue(requestparam, "placemend_id");

	adstyle_s = getParamvalue(requestparam, "adstyle");
	if (adstyle_s != NULL)
	{
		writeLog(g_logid_local, LOGINFO, "id: %s and adstyle %s", crequest.id.c_str(), adstyle_s);
		adstyle = atoi(adstyle_s);
		if (adstyle == 1)
		{
			cimp.type = IMP_BANNER;
			cimp.banner.w = 1920;
			cimp.banner.h = 1080;
		}
		else if (adstyle == 2)
		{
			cimp.type = IMP_BANNER;
			cimp.banner.w = 640;
			cimp.banner.h = 960;
		}
		else
		{
			err = E_REQUEST_IMP_TYPE;
		}
	}
	else
	{
		err = E_REQUEST_IMP_SIZE_ZERO;
	}

	pos_s = getParamvalue(requestparam, "pos");
	if (pos_s != NULL)
	{
		cimp.adpos = atoi(pos_s);
	}

	/*adtype_s = getParamvalue(requestparam, "adtype");

	if(adtype_s != NULL)
	{
	writeLog(g_logid_local, LOGINFO, "id: %s and adtype %s", crequest.id.c_str(), adtype_s);
	adtype = atoi(adtype_s);
	if(adtype == 3)
	{
	cimp.type = IMP_BANNER;
	cimp.ext.splash = 1;
	char *scrwidth  = NULL;
	char *scrheight  = NULL;
	scrwidth  = getParamvalue(requestparam, "scrwidth");
	scrheight  = getParamvalue(requestparam, "scrheight");
	if(scrwidth  != NULL && scrheight != NULL)
	{
	cimp.banner.w = atoi(scrwidth);
	cimp.banner.h = atoi(scrheight);
	}
	FREE(scrwidth);
	FREE(scrheight);
	}
	}*/

	//tradelevel = getParamvalue(requestparam, "tradelevel");
	//待转化

	net_s = getParamvalue(requestparam, "net");
	if (net_s != NULL)
	{
		int net = atoi(net_s);
		crequest.device.connectiontype = net;
	}

	carrier_s = getParamvalue(requestparam, "carrier");
	if (carrier_s != NULL)
	{
		crequest.device.carrier = atoi(carrier_s);
	}

	os_s = getParamvalue(requestparam, "os");
	if (os_s != NULL)
	{
		int os = atoi(os_s);
		switch (os)
		{
		case 2:
		{
			os = OS_ANDROID;
			/*imei = getParamvalue(requestparam, "imei");
			if(imei != NULL)
			{
			crequest.device.dids.insert(make_pair(DID_IMEI, imei));
			}*/
			andid = getParamvalue(requestparam, "andid");
			if (andid != NULL)
			{
				crequest.device.dpids.insert(make_pair(DPID_ANDROIDID, andid));
			}
			break;
		}
		case 1:
		{
			os = OS_IOS;
			idfa = getParamvalue(requestparam, "idfa");
			if (idfa != NULL)
			{
				crequest.device.dpids.insert(make_pair(DPID_IDFA, idfa));
			}
			break;
		}
		case 3:
		{
			os = OS_WINDOWS;
			break;
		}
		}
		crequest.device.os = os;
	}

	devicetype_s = getParamvalue(requestparam, "devicetype");
	if (devicetype_s != NULL)
	{
		crequest.device.devicetype = atoi(devicetype_s);
	}

	osv = getParamvalue(requestparam, "osv");
	if (osv != NULL)
	{
		crequest.device.osv = osv;
	}


	make_s = getParamvalue(requestparam, "brand");
	if (make_s != NULL)
	{
		string s_make = make_s;
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

	ua = getParamvalue(requestparam, "ua");
	if (ua != NULL)
	{
		crequest.device.ua = ua;
	}

	crequest.device.ip = reqaddr;

#ifdef DEBUG
	istest = getParamvalue(requestparam, "is_test");
	if (istest != NULL)
	{
		if (istest[0] == '1')
		{
			crequest.device.ip = "60.24.0.0";
		}
	}
#endif

	int location = getRegionCode(geodb, crequest.device.ip);
	crequest.device.location = location;

	wma = getParamvalue(requestparam, "mac");
	if (wma != NULL)
	{
		crequest.device.dids.insert(make_pair(DID_MAC, wma));
	}

	lat = getParamvalue(requestparam, "lat");
	lon = getParamvalue(requestparam, "lon");
	if (lat != NULL && lon != NULL)
	{
		crequest.device.geo.lat = atoi(lat);
		crequest.device.geo.lon = atoi(lon);
	}
	crequest.imp.push_back(cimp);

	crequest.app.id = "hometv";
	crequest.app.name = crequest.app.id;

exit:
	FREE(id);
	FREE(adid);
	FREE(adtype_s);
	FREE(adstyle_s);
	FREE(net_s);
	FREE(carrier_s);
	FREE(os_s);
	FREE(devicetype_s);
	FREE(osv);
	FREE(make_s);
	//	FREE(imei);
	FREE(ua);
	FREE(wma);
	FREE(andid);
	FREE(idfa);
	FREE(lat);
	FREE(lon);
	FREE(pos_s);
	FREE(istest);
	return err;
}

static void setHomeTVJsonResponse(COM_REQUEST &request, COM_RESPONSE &cresponse, ADXTEMPLATE &adxtemp, string &data_out, int errcode)
{
	char *text = NULL;
	json_t *root, *obj_data, *label_data, *array_data;

	string trace_url_par = string("&") + get_trace_url_par(request, cresponse);
	root = json_new_object();
	if (errcode != E_SUCCESS)
	{
		jsonInsertInt(root, "code", 204);
		goto exit;
	}

	jsonInsertInt(root, "code", 200);
	if (cresponse.seatbid.size() > 0)
	{
		label_data = json_new_string("ads");
		COM_SEATBIDOBJECT &seatbid = cresponse.seatbid[0];
		array_data = json_new_array();
		obj_data = json_new_object();
		if (seatbid.bid.size() > 0)
		{
			COM_BIDOBJECT &bid = seatbid.bid[0];
			string cturl;
			string iurl;
			if (bid.type == ADTYPE_FEEDS)
			{
				for (int i = 0; i < bid.native.assets.size(); ++i)
				{
					COM_ASSET_RESPONSE_OBJECT &comasetrepobj = bid.native.assets[i];
					if (comasetrepobj.required == 1)
					{
						switch (comasetrepobj.type)
						{
						case NATIVE_ASSETTYPE_TITLE:
						{
							jsonInsertString(obj_data, "adtitle", comasetrepobj.title.text.c_str());
							break;
						}
						/*case NATIVE_ASSETTYPE_IMAGE:
						{
						jsonInsertString(root, "imgurl", comasetrepobj.img.url.c_str());
						break;
						}*/
						case NATIVE_ASSETTYPE_DATA:
						{
							jsonInsertString(obj_data, "adtext", comasetrepobj.data.value.c_str());
							break;
						}
						}
					}
				}
			}
			else if (bid.type == ADTYPE_IMAGE)
			{
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
					}
					if (bid.curl != "")
					{
						string curl = bid.curl;

						replaceMacro(adm, "#CURL#", curl.c_str());
						if (adxtemp.cturl.size() > 0)
						{
                            cturl = adxtemp.cturl[0] + trace_url_par; 
							replaceMacro(adm, "#CTURL#", cturl.c_str());
						}
						if (adxtemp.iurl.size() > 0)
						{
							iurl = adxtemp.iurl[0] + trace_url_par;
							replaceMacro(adm, "#IURL#", iurl.c_str());
						}
						string monitorcode = bid.monitorcode;
						replaceMacro(adm, "#MONITORCODE#", monitorcode.c_str());
						if (bid.cmonitorurl.size() > 0)
						{
							string cmonitor = bid.cmonitorurl[0];
							replaceMacro(adm, "#CMONITORURL#", cmonitor.c_str());
						}
					}
					jsonInsertString(obj_data, "adm", adm.c_str());
				}
			}

			jsonInsertInt(obj_data, "adwidth", bid.w);
			jsonInsertInt(obj_data, "adheight", bid.h);

			jsonInsertInt(obj_data, "admt", bid.type);
			jsonInsertInt(obj_data, "adct", bid.ctype);
		}
		json_insert_child(array_data, obj_data);
		json_insert_child(label_data, array_data);
		json_insert_child(root, label_data);
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

static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if (price - crequest.imp[0].bidfloor - 0.01 > 0.000001)
		return true;

	return false;
}

int getBidResponse(IN  uint64_t ctx, IN int index, IN char *requestaddr,
	IN char *requestparam, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int err = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	uint32_t sendlength = 0;
	COM_REQUEST crequest;//common msg request
	string output;
	int ret;
	COM_RESPONSE cresponse;
	string outputdata;//, senddata;
	ADXTEMPLATE adxtemp;
	struct timespec ts1, ts2;
	long lasttime;
	string str_ferrcode;

	getTime(&ts1);
	init_com_message_request(&crequest);

	err = parseHomeTVRequest(requestparam, requestaddr, crequest);
	if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", requestparam);
		goto set_result;
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
		uint64_t cm;
		char strcm[16] = "";
		cm = ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000;
		snprintf(strcm, 16, "%llu", cm);
		crequest.id = string(strcm) + crequest.id;
		crequest.at = BID_PMP;

		init_com_message_response(&cresponse);
		init_ADXTEMPLATE(adxtemp);

		//ferrcode = bid_filter_response(ctx, index, crequest, NULL, NULL, callback_check_price, NULL, &cresponse, &adxtemp);	
		ferrcode = bid_filter_response(ctx, index, crequest, NULL, NULL, NULL, NULL, &cresponse, &adxtemp);
		err = ferrcode.errcode;
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		flow_bid.set_bid_flow(ferrcode);
		g_ser_log.send_bid_log(index, crequest, cresponse, err);
		setHomeTVJsonResponse(crequest, cresponse, adxtemp, output, err);
	}

set_result:
	senddata = string("Content-Type: application/json;charset=UTF-8\r\nContent-Length: ")
		+ intToString(output.size()) + "\r\n\r\n" + output;

	if (err == E_SUCCESS)
	{
		numcount[index]++;
		writeLog(g_logid, LOGINFO, "%s,%s", requestaddr, requestparam);
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d", cresponse.id.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(),
			crequest.device.deviceid.c_str(), crequest.device.deviceidtype);
		if (numcount[index] % 1000 == 0)
		{
			writeLog(g_logid_local, LOGDEBUG, "response :" + output);
			numcount[index] = 0;
		}
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x", crequest.id.c_str(), lasttime, err);

exit:
	flow_bid.set_err(err);
	return err;
}
