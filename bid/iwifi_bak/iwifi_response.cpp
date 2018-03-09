/*
 * iwifi_response.cpp
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

// #include "../common/setlog.h"
// #include "../common/tcp.h"
#include "../../common/json_util.h"
#include "iwifi_response.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/bid_filter_type.h"

extern int *numcount;
extern bool fullreqrecord;

extern uint64_t g_logid, g_logid_local;

char *uintToStr(int arg)
{
	char num[16];

	sprintf(num, "%u", arg);

	return num;
}

/*void init_publisher_object(PUBLISHEROBJECT &publisher)//
{
	publisher.id = publisher.name = publisher.domain = "";
}

void init_producer_object(PRODUCEROBJECT &producer)//
{
	init_publisher_object((PUBLISHEROBJECT&)producer);
}

void init_content_object(CONTENTOBJECT &content)//
{
	content.id = content.title = content.series = content.season = content.url = content.keywords = 
		content.contentrating = content.userrating = content.context = "";

	content.episode = content.videoquality = content.livestream = content.sourcerelationship = content.len;

	init_producer_object(content.producer);
}

void init_geo_object(GEOOBJECT &geo)//
{
	geo.lat = geo.lon = geo.type = 0;

	geo.country = geo.region = geo.regionfips104 = geo.metro = geo.city = geo.zip = "";
}

void init_site_object(SITEOBJECT &site)//
{
	site.id = site.name = site.domain = site.ref = site.search = site.keywords = "";

	site.privacypolicy = 0;

	init_publisher_object(site.publisher);

	init_content_object(site.content);
}
*/
void init_app_object(APPOBJECT &app)//
{
	 app.ver = app.itid = app.Keywords = app.storeurl = app.Pid = app.pub = "";

	 app.paid = 0;

	//init_publisher_object(app.publisher);

	//init_content_object(app.content);
}

void init_device_object(DEVICEOBJECT &device)//
{
	device.os = device.devicetype = device.sw = device.sh = device.orientation  = device.density = 0;

	
	device.province = device.city = device.county = device.dpid = device.mac = device.carrier = device.language=  device.loc= "";
}

void init_message_request(MESSAGEREQUEST &mrequest)//
{
	mrequest.bidfloorcur = "CNY";

	//init_site_object(mrequest.site);

	init_app_object(mrequest.app);

	init_device_object(mrequest.device);

	//mrequest.at = 2;
}
/*
void init_banner_ext(BANNEREXT &ext)
{
	ext.adh = ext.orientationlock = 0;

	ext.orientation = "";
}

void init_banner_object(BANNEROBJECT &banner)
{
	banner.w = banner.h = banner.pos = banner.topframe = 0;

	banner.id = "";

	init_banner_ext(banner.ext);
}
*/
void init_impression_object(IMPRESSIONOBJECT &imp)
{
	//imp.id = imp.tagid = "";
	imp.emb = imp.adtype = imp.page = imp.pos = 0;
	imp.bidfloorcur = "CNY";
}
/*
void init_seat_bid(SEATBIDOBJECT &seatbid)
{
	seatbid.seat = "";
}

void init_bid_object(BIDOBJECT &bid)
{

}

void init_message_response(MESSAGERESPONSE &response)
{
	response.id = "";
	response.cur = "USD";
}
*/
static int parseIwifiRequest(RECVDATA *recvdata, MESSAGEREQUEST &request, COM_REQUEST &crequest)
{
	json_t *root = NULL, *label;

#ifdef _DEBUG
	writeLog(g_logid_local, "Begin parseAmaxRequest");
#endif
	if (recvdata->length == 0)
		return E_BAD_REQUEST;

	json_parse_document(&root, recvdata->data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseAmaxRequest root is NULL!");
		return E_BAD_REQUEST;
	}

#ifdef _DEBUG
	writeLog(g_logid_local, LOGINFO, "Parse Amax Request success!");
#endif
	
	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		crequest.id = label->child->text;
		request.id = label->child->text;
	}

	if ((label = json_find_first_label(root, "version")) != NULL)
		request.version = label->child->text;

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *value_imp = label->child->child;

		if (value_imp != NULL)
		{
			IMPRESSIONOBJECT imp;
			COM_IMPRESSIONOBJECT cimp;

			init_com_impression_object(&cimp);
			cimp.imptype = IMP_BANNER;
			
			if ((label = json_find_first_label(value_imp, "impid")) != NULL)
				cimp.id = label->child->text;

			if ((label = json_find_first_label(value_imp, "emb")) != NULL && label->child->type == JSON_NUMBER)
			{
				imp.emb = atoi(label->child->text);
				//只接收APP广告请求
				if(imp.emb != 1)
					return E_INVALID_PARAM;
			}

			if ((label = json_find_first_label(value_imp, "adtype")) != NULL && label->child->type == JSON_NUMBER)
			{//需要比对字段决定是否需要转换
				imp.adtype = atoi(label->child->text);
			}

			if ((label = json_find_first_label(value_imp, "bidfloor")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				imp.bidfloor = atof(label->child->text);
				cimp.bidfloor = imp.bidfloor /100;
			}

			if ((label = json_find_first_label(value_imp, "bidfloorcur")) != NULL)
				request.bidfloorcur = label->child->text;

			if ((label = json_find_first_label(value_imp, "width")) != NULL
				&& label->child->type == JSON_NUMBER)
				cimp.banner.w = atoi(label->child->text);

			if ((label = json_find_first_label(value_imp, "height")) != NULL
				&& label->child->type == JSON_NUMBER)
				cimp.banner.h = atoi(label->child->text);

			if ((label = json_find_first_label(value_imp, "page")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				imp.page = atoi(label->child->text);
			}

			if ((label = json_find_first_label(value_imp, "pos")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				imp.pos = atoi(label->child->text);
			}

			if ((label = json_find_first_label(value_imp, "btype")) != NULL)
			{
				json_t *temp = label->child->child;
				//需要转换映射，待讨论
				while (temp != NULL)
				{
					cimp.banner.btype.insert(atoi(temp->text));
					temp = temp->next;
				}
			}
			if ((label = json_find_first_label(value_imp, "battr")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					cimp.banner.battr.insert(atoi(temp->text));
					temp = temp->next;
				}
			}

			request.imps.push_back(imp);
			crequest.imp.push_back(cimp);
		}
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "aid")) != NULL)
			crequest.app.id = label->child->text;
		if ((label = json_find_first_label(temp, "name")) != NULL)
			crequest.app.name = label->child->text;
		if ((label = json_find_first_label(temp, "cat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				crequest.app.cat.insert(atoi(temp->text));
				temp = temp->next;
			}
		}
		if ((label = json_find_first_label(temp, "ver")) != NULL)
			request.app.ver = label->child->text;

		if ((label = json_find_first_label(temp, "itid")) != NULL)
			request.app.itid = label->child->text;
		if ((label = json_find_first_label(temp, "paid")) != NULL
			&& label->child->type == JSON_NUMBER)
			request.app.paid = atoi(label->child->text);
		if ((label = json_find_first_label(temp, "storeurl")) != NULL)
			request.app.storeurl = label->child->text;
		if ((label = json_find_first_label(temp, "Keywords")) != NULL)
			request.app.Keywords = label->child->text;
		if ((label = json_find_first_label(temp, "Pid")) != NULL)
			request.app.Pid = label->child->text;
		if ((label = json_find_first_label(temp, "pub")) != NULL)
			request.app.pub = label->child->text;
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "did")) != NULL)
		{
			crequest.device.did = label->child->text;
			crequest.device.didtype = DID_IMEI_SHA1;
		}

		
		if ((label = json_find_first_label(temp, "province")) != NULL)
			request.device.province = label->child->text;
		if ((label = json_find_first_label(temp, "city")) != NULL)
			request.device.city = label->child->text;
		if ((label = json_find_first_label(temp, "county")) != NULL)
			request.device.county = label->child->text;

		if ((label = json_find_first_label(temp, "dpid")) != NULL)
		{
			crequest.device.dpid = label->child->text;
			//request.device.dpid = label->child->text;
		}

		if ((label = json_find_first_label(temp, "mac")) != NULL)
			request.device.mac = label->child->text;

		if ((label = json_find_first_label(temp, "ua")) != NULL)
			crequest.device.ua = label->child->text;

		if ((label = json_find_first_label(temp, "userip")) != NULL)
			crequest.device.ip = label->child->text;

		if ((label = json_find_first_label(temp, "carrier")) != NULL)
		{
			request.device.carrier = label->child->text;
			int carrier = CARRIER_UNKNOWN;
			carrier = atoi(request.device.carrier.c_str());
			if(carrier == 46000 || carrier == 46002 || carrier == 46007)
				crequest.device.carrier = CARRIER_CHINAMOBILE;
			else if(carrier == 46001 || carrier == 46006)
				crequest.device.carrier = CARRIER_CHINAUNICOM;
			else if(carrier == 46003 || carrier == 46005)
				crequest.device.carrier = CARRIER_CHINATELECOM;

		}
		if ((label = json_find_first_label(temp, "language")) != NULL)
			request.device.language = label->child->text;
		if ((label = json_find_first_label(temp, "make")) != NULL)
			crequest.device.make = label->child->text;
		if ((label = json_find_first_label(temp, "model")) != NULL)
			crequest.device.model = label->child->text;

		if ((label = json_find_first_label(temp, "os")) != NULL)
		{
			//将os转为int
			if(strcasecmp(label->child->text, "iPhone OS") == 0)
			{
				crequest.device.os = 1;
				request.device.os = 1;
				crequest.device.dpidtype = DPID_IDFA;	
			}
			else if(strcasecmp(label->child->text, "Android") == 0)
			{
				crequest.device.os = 2;
				request.device.os = 2;
				crequest.device.dpidtype = DPID_ANDROIDID;		
				
			}
			else if(strcasecmp(label->child->text, "Windows") == 0)
			{
				crequest.device.os = 3;
				request.device.os = 3;
				crequest.device.dpidtype = DPID_OTHER;
			}
		}
			
		if ((label = json_find_first_label(temp, "osv")) != NULL)
			crequest.device.osv = label->child->text;

		if ((label = json_find_first_label(temp, "connectiontype")) != NULL
			&& label->child->type == JSON_NUMBER)
			crequest.device.connectiontype = atoi(label->child->text);

		if ((label = json_find_first_label(temp, "devicetype")) != NULL
			&& label->child->type == JSON_NUMBER)
		{
			//devicetype需映射
			request.device.devicetype = atoi(label->child->text);
			switch(request.device.devicetype)
			{
			case 1:
			case 2:
			case 4:
				crequest.device.devicetype = 1;
				break;
			case 3:
			case 5:
				crequest.device.devicetype = 2;
				break;
			case 6:
				crequest.device.devicetype = 3;
				break;
			default:
				crequest.device.devicetype = 0;
				break;
			}
			
		}

		if ((label = json_find_first_label(temp, "loc")) != NULL)
		{
			char *p = NULL;
			request.device.loc = label->child->text;
			p = strtok(label->child->text, ",");
			if(p)
				crequest.device.geo.lat = atof(p);
			p=strtok(NULL, ",");
			if(p)
				crequest.device.geo.lon = atof(p);
		}
		if ((label = json_find_first_label(temp, "density")) != NULL)
			request.device.density = atof(label->child->text);
		if ((label = json_find_first_label(temp, "sw")) != NULL
			&& label->child->type == JSON_NUMBER)
			request.device.sw = atoi(label->child->text);
		if ((label = json_find_first_label(temp, "sh")) != NULL
			&& label->child->type == JSON_NUMBER)
			request.device.sh = atoi(label->child->text);
		if ((label = json_find_first_label(temp, "orientation")) != NULL
			&& label->child->type == JSON_NUMBER)
			request.device.orientation = atoi(label->child->text);
	}

	crequest.cur = "CNY";

	if ((label = json_find_first_label(root, "bcat")) != NULL)
	{
		json_t *temp = label->child->child;
		//需通过映射精确到二类
		while (temp != NULL)
		{
			request.bcat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.badv.push_back(temp->text);
			temp = temp->next;
		}
	}
	json_free_value(&root);

#ifdef _DEBUG
	writeLog(g_logid_local, "End parseAmaxRequest");
#endif
	return 0;
}

static bool setIwifiJsonResponse(MESSAGEREQUEST &request, COM_RESPONSE &cresponse, ADXTEMPLATE &adxtemp, string &data_out, int flag)
{
	char *text;
	json_t *root, *label_seatbid, *label_bid, *label_ext,
		*array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;
	COM_BIDOBJECT *bid;
	COM_SEATBIDOBJECT *seatbid = NULL;
#ifdef _DEBUG
	writeLog(g_logid_local, "Begin setIwifiJsonResponse");
#endif
	root = json_new_object();
	jsonInsertString(root, "id", request.id.c_str());
	
	if (flag != 0)
	{
		jsonInsertInt(root, "nbr", 0);
		goto exit;
	}

	if (cresponse.bidid.size() > 0)
		jsonInsertString(root, "bidid", cresponse.bidid.c_str());

	//label_bid = json_new_string("bid");
	seatbid = &cresponse.seatbid[i];
	if (!seatbid->bid.empty() && seatbid->bid.size() > 0)
	{
		label_bid = json_new_string("bid");
		//array_bid = json_new_array();
		bid = &cresponse.seatbid[0].bid[0];
		entry_bid = json_new_object();
		jsonInsertString(entry_bid, "impid", bid->impid.c_str());
		if(adxtemp.ratio != 0)
			jsonInsertInt(entry_bid, "price", bid->price * 100 * adxtemp.ratio);
		else
			jsonInsertInt(entry_bid, "price", bid->price * 100);

		jsonInsertString(entry_bid, "adid", bid->adid.c_str());

		if (!adxtemp.nurl.empty() && adxtemp.nurl.size() > 0)
		{
			string nurl = adxtemp.nurl;
			replaceMacro(nurl, "#BID#", cresponse.id.c_str());
			replaceMacro(nurl, "#MAPID#", bid->adid.c_str());
			if (!request.device.deviceid.empty() && request.device.deviceid.size() > 0)
			{
				replaceMacro(nurl, "#DEVICEID#", request.device.deviceid.c_str());
				replaceMacro(nurl, "#DEVICEIDTYPE#", intToString(request.device.deviceidtype).c_str());
			}
			jsonInsertString(entry_bid, "nurl", nurl.c_str());
		}
		if (!adxtemp.adms.empty() && adxtemp.adms.size() > 0)
		{
			string adm;
			//根据os和type查找对应的adm
			int count = 0;
			bool found = false;
			for(; count < adxtemp.adms.size(); count++)
			{
				//cout << v[i] << endl;
				if(adxtemp.adms[count].os == request.device.os && adxtemp.adms[count].type == bid->type)
				{
					found = true;
					adm = adxtemp.adms[count].adm;
					break;
				}
			}

			if(found)
			{
				replaceMacro(adm, "#SOURCEURL#", bid->sourceurl.c_str());
				//IURL需要替换
				if(adxtemp.iurl.size() > 0)
				{
					string iurl = adxtemp.iurl[0];

					replaceMacro(iurl, "#MAPID#", bid->adid.c_str());
					replaceMacro(iurl, "#BID#", cresponse.id.c_str());
					if (!request.device.deviceid.empty() && request.device.deviceid.size() > 0)
					{
						replaceMacro(iurl, "#DEVICEID#", request.device.deviceid.c_str());
						replaceMacro(iurl, "#DEVICEIDTYPE#", intToString(request.device.deviceidtype).c_str());
					}
					replaceMacro(adm, "#IURL#", iurl.c_str());
				}
				jsonInsertString(entry_bid, "adm", adm.c_str());
			}

		}

		/*dspSourceg固定，存到文件或代码中
		jsonInsertString(entry_bid, "dspSource", bid->dspSource.c_str());*/
		if(bid->exts.size() > 0)
		{
			map<uint8_t, string >::const_iterator ext_it  = bid->exts.find(ADX_IWIFI);
			if(ext_it != bid->exts.end())
			{
				string extinfo = ext_it->second;
				json_t *tmproot = NULL;
				json_t *label;
				json_parse_document(&tmproot, extinfo.c_str());
				if(tmproot != NULL)
				{
					if((label = json_find_first_label(tmproot, "docid")) != NULL)
						jsonInsertString(entry_bid, "docid", label->child->text);
					json_free_value(&tmproot);
				}
			}	
		}

		if (bid->w > 0)
			jsonInsertInt(entry_bid, "adwidth", bid->w);
		if (bid->h > 0)
			jsonInsertInt(entry_bid, "adheight", bid->h);

		if (!adxtemp.iurl.empty() && adxtemp.iurl.size() > 0)
		{
			string iurl = adxtemp.iurl[0];
			replaceMacro(iurl, "#BID#", cresponse.id.c_str());
			replaceMacro(iurl, "#MAPID#", bid->adid.c_str());
			if (!request.device.deviceid.empty() && request.device.deviceid.size() > 0)
			{
				replaceMacro(iurl, "#DEVICEID#", request.device.deviceid.c_str());
				replaceMacro(iurl, "#DEVICEIDTYPE#", intToString(request.device.deviceidtype).c_str());
			}
			jsonInsertString(entry_bid, "iurl", iurl.c_str());
		}
		jsonInsertString(entry_bid, "curl", bid->curl.c_str());

		if (!adxtemp.cturl.empty() && adxtemp.cturl.size() > 0)
		{
			string cturl = adxtemp.cturl[0];
			replaceMacro(cturl, "#BID#", cresponse.id.c_str());
			replaceMacro(cturl, "#MAPID#", bid->adid.c_str());
			if (!request.device.deviceid.empty() && request.device.deviceid.size() > 0)
			{
				replaceMacro(cturl, "#DEVICEID#", request.device.deviceid.c_str());
				replaceMacro(cturl, "#DEVICEIDTYPE#", intToString(request.device.deviceidtype).c_str());
			}

			jsonInsertString(entry_bid, "cturl", cturl.c_str());
		}

		if (!adxtemp.aurl.empty() && adxtemp.aurl.size() > 0)
		{
			string aurl;
			replaceMacro(aurl, "#BID#", cresponse.id.c_str());
			replaceMacro(aurl, "#MAPID#", bid->adid.c_str());
			if (!request.device.deviceid.empty() && request.device.deviceid.size() > 0)
			{
				replaceMacro(aurl, "#DEVICEID#", request.device.deviceid.c_str());
				replaceMacro(aurl, "#DEVICEIDTYPE#", intToString(request.device.deviceidtype).c_str());
			}
			jsonInsertString(entry_bid, "aurl", aurl.c_str());
		}
		if (!bid->cid.empty() && bid->cid.size() > 0)
			jsonInsertString(entry_bid, "cid", bid->cid.c_str());
		if (!bid->crid.empty() && bid->crid.size() > 0)
			jsonInsertString(entry_bid, "crid", bid->crid.c_str());
		if (bid->ctype > 0)
			jsonInsertInt(entry_bid, "ctype", bid->ctype);
		if (bid->ctype == 2 &&  bid->bundle.size() > 0)
			jsonInsertString(entry_bid, "cbundle", bid->bundle.c_str());

		if (!bid->adomain.empty() && bid->adomain.size() > 0)
			jsonInsertString(entry_bid, "adomain", bid->adomain.c_str());

		//json_insert_child(array_bid, entry_bid);
		json_insert_child(label_bid, entry_bid);
		//json_insert_child(label_bid, array_bid);
		json_insert_child(root, label_bid);
	}

exit:
	json_tree_to_string(root, &text);
	data_out = text;
	free(text);
	json_free_value(&root);

#ifdef _DEBUG
	writeLog(g_logid_local, "End setIwifiJsonResponse");
#endif
}

static bool adapter_callback(COM_BIDOBJECT *bid)
{
	if(bid->exts.size() > 0)
	{
		map<uint8_t, string >::const_iterator ext_it  = bid->exts.find(ADX_IWIFI);
		if(ext_it != bid->exts.end())
		{
			string extinfo = ext_it->second;
			json_t *tmproot = NULL;
			json_t *label;
			json_parse_document(&tmproot, extinfo.c_str());
			if(tmproot != NULL)
			{
				if((label = json_find_first_label(tmproot, "docid")) != NULL)
				{
					if(strlen(label->child->text) != 0)
					{
						json_free_value(&tmproot);
						return true;
					}
					json_free_value(&tmproot);
				}
			}
		}	
		
	}
	return false;
}


int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata)
{
	int err = 0;
	uint32_t sendlength = 0;
	MESSAGEREQUEST mrequest;//adx msg request
	COM_REQUEST crequest;//common msg request
	MESSAGERESPONSE mresponse;
	COM_RESPONSE cresponse;
	ADXTEMPLATE adxtemp;
	struct timespec ts1, ts2;
	long lasttime;
	string output = "";
	int ret;

	string outputdata;

	if (recvdata == NULL)
	{
		err = -1;
		goto exit;
	}

	if ((recvdata->data == NULL)||(recvdata->length == 0))
	{
		err = -1;
		goto exit;
	}

	init_message_request(mrequest);
	init_com_message_request(&crequest);
	ret = parseIwifiRequest(recvdata, mrequest, crequest);
	if(ret == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);
		goto exit;
	}
	if (ret == E_SUCCESS)
	{
		init_com_message_response(&cresponse);
		err = bid_filter_response(ctx, index, crequest, NULL, NULL, NULL, &cresponse, &adxtemp);	
		mrequest.device.deviceid = crequest.device.deviceid;
		mrequest.device.deviceidtype = crequest.device.deviceidtype;
	}
	else if(ret == E_INVALID_PARAM)
		err = 1;

	setIwifiJsonResponse(mrequest, cresponse, adxtemp, output, err);
	senddata = "Content-Type: application/json\r\nx-iwifirtb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

	if (ret != E_BAD_REQUEST)//竞价打印本地日志
	{
		sendLog(datatransfer, crequest, ADX_AMAX, err);
		if(err == 0)
		{
			numcount[index]++;
			writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
			if(numcount[index] %1000 == 0)
			{
				writeLog(g_logid_local, LOGDEBUG, "response :" + output);
				numcount[index] = 0;
			}
		}
		else if(fullreqrecord)
			writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));

		getTime(&ts2);
		lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
		writeLog(g_logid_local, LOGDEBUG, "%s, spent time:%ld, err:0x%x, tid:0x%x", mrequest.id.c_str(), lasttime, err, datatransfer->threadlog);
		va_cout("%s, spent time:%ld, err:0x%x, tid:0x%x",  mrequest.id.c_str(), lasttime, err, datatransfer->threadlog);
	}

	err = 0;
exit:

	return err;
}




