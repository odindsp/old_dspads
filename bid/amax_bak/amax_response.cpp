/*
 * amax_response.cpp
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
#include "amax_response.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"

#define ADS_AMOUNT 20
extern map<string, vector<uint32_t> > app_cat_table;
extern map<string, vector<uint32_t> > adv_cat_table_adxi;
extern map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
extern map<string, uint16_t> dev_make_table;
extern int *numcount;
extern uint64_t geodb;
extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local, g_logid_response;
static string g_ads_size[] = {"320x50", "640x100", "728x90", "1456x180", "468x60", "936x120",
	"300x250", "600x500", "640x1136", "1136x640", "480x700", "700x480", "768x1024",
	"1024x768", "560x750", "750x560", "240x290", "400x208", "1280x720", "780x400"};
//static uint32_t g_ads_type[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};
static uint32_t g_ads_enlarge[] = {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint32_t g_ads_sizeratio[] = {5634, 6857, 7467, 7500, 8276, 12000, 12000, 13333, 13393,
	14583, 17750, 17778, 19231, 19500, 64000, 64000, 78000, 78000, 80889, 80889};
static uint32_t g_ads_sizeratio_upgrade_index[] = {8, 10, 14, 12, 16, 6, 7, 13, 15, 11, 9, 18, 17, 19, 0, 1, 4, 5, 2, 3};

extern MD5_CTX hash_ctx; 

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
void init_app_object(APPOBJECT &app)
{
	
}


void init_device_object(DEVICEOBJECT &device)//
{
	device.deviceidtype = device.devicetype = device.sw = device.sh = device.orientation  = device.os= 0;

	//device.did = device.dpid = device.mac = device.loc = device.carrier =  "";
}

void init_message_request(MESSAGEREQUEST &mrequest)//
{
	//mrequest.id = "";

	//init_site_object(mrequest.site);

	//init_app_object(mrequest.app);

	init_device_object(mrequest.device);

	mrequest.instl = mrequest.splash = 0;
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
	//imp.id = "";
	
	//imp.bidfloorcur = "CNY";

	imp.bidfloor = imp.instl = imp.splash = 0;

	//init_banner_object(imp.banner);
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

int parseAmaxRequest(char *data, MESSAGEREQUEST &request, COM_REQUEST &crequest)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;

	if(data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseAmaxRequest root is NULL!");
		return E_BAD_REQUEST;
	}
	
	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		crequest.id = label->child->text;
		request.id = label->child->text;
	}
	else
	{
		err =  E_BAD_REQUEST;
		goto exit;
	}

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *value_imp = label->child->child;

		if (value_imp != NULL)
		{
			IMPRESSIONOBJECT imp;
			COM_IMPRESSIONOBJECT cimp;

			init_impression_object(imp);
			init_com_impression_object(&cimp);

			cimp.type = IMP_BANNER;
			cimp.bidfloorcur = "CNY";
			if ((label = json_find_first_label(value_imp, "impid")) != NULL)
				cimp.id = label->child->text;
			else
			{
				err =  E_BAD_REQUEST;
				goto exit;
			}
			if ((label = json_find_first_label(value_imp, "bidfloor")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				
				imp.bidfloor = atoi(label->child->text);
				cimp.bidfloor = imp.bidfloor /10000.0;
			}

			if ((label = json_find_first_label(value_imp, "bidfloorcur")) != NULL)
				request.bidfloorcur = label->child->text;

			if ((label = json_find_first_label(value_imp, "w")) != NULL
				&& label->child->type == JSON_NUMBER)
				cimp.banner.w = atoi(label->child->text);

			if ((label = json_find_first_label(value_imp, "h")) != NULL
				&& label->child->type == JSON_NUMBER)
				cimp.banner.h = atoi(label->child->text);

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

			if ((label = json_find_first_label(value_imp, "instl")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				imp.instl = atoi(label->child->text);
				cimp.ext.instl = imp.instl;
			}

			if ((label = json_find_first_label(value_imp, "splash")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				imp.splash = atoi(label->child->text);
				cimp.ext.splash = imp.splash;
				if(imp.splash)
				{
					//不支持开屏广告，直接返回nbr为0
					err = E_INVALID_PARAM;
				}
			}

			request.imps.push_back(imp);
			crequest.imp.push_back(cimp);
		}
	}
	else
	{
		err =  E_BAD_REQUEST;
		goto exit;
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "aid")) != NULL)
		{
			crequest.app.id = label->child->text;
			//临时屏蔽暴风的流量
			/*if(crequest.app.id == "4101856e6dba4778a73b0cbe0cb5dad9")
				err = E_INVALID_PARAM;*/
		}
		if ((label = json_find_first_label(temp, "name")) != NULL)
			crequest.app.name = label->child->text;
		if ((label = json_find_first_label(temp, "cat")) != NULL)
		{
			//待映射
			json_t *temp = label->child->child;
			while (temp != NULL)
			{
				//奥丁版本增加了cat定向，adapter不需要对app.cat进行过滤
				/*int  cat = atoi(temp->text);
				if(cat == 60001 || cat ==  60501)
				{
					//驾校一点通的cat中60001或60005
					if(crequest.app.id != "bf90666fc027407bb847050f4a3dbfb0")
					{
						err = E_INVALID_PARAM;
					}
					//writeLog(g_logid_local, LOGDEBUG, "invalid cat ios book");
					//日志服务器需要所有的cat种类，不应该break
					//break;
				}*/

				vector<uint32_t> &v_cat = app_cat_table[temp->text];
				for (uint32_t i = 0; i < v_cat.size(); ++i)
					crequest.app.cat.insert(v_cat[i]);
				
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(temp, "bundle")) != NULL)
		{
			crequest.app.bundle = label->child->text;
			request.app.bundle = label->child->text;
		}

		if ((label = json_find_first_label(temp, "storeurl")) != NULL)
			crequest.app.storeurl = label->child->text;
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *temp = label->child;
		string dpid, did;
		dpid = did = "";
		uint8_t dpidtype = DPID_UNKNOWN;
		if ((label = json_find_first_label(temp, "did")) != NULL)
		{
			 did = label->child->text;
			//request.device.did = did;
			 if(did != "")
			 {
				 transform(did.begin(), did.end(), did.begin(), ::tolower);
				 crequest.device.dids.insert(make_pair(DID_IMEI_SHA1, did));
			 }
		}
		
		if ((label = json_find_first_label(temp, "dpid")) != NULL)
		{
			dpid = label->child->text;
			//request.device.dpid = label->child->text;
		}

		if ((label = json_find_first_label(temp, "mac")) != NULL)
		{
			string mac = label->child->text;
			if(mac != "")
			{
				request.device.mac = mac;
				transform(mac.begin(), mac.end(), mac.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_MAC_SHA1, mac));
			}
		}
		
		if ((label = json_find_first_label(temp, "ua")) != NULL)
			crequest.device.ua = label->child->text;
		if ((label = json_find_first_label(temp, "ip")) != NULL)
		{
			crequest.device.ip = label->child->text;
			if(crequest.device.ip.size() > 0)
			{
				int location = getRegionCode(geodb, crequest.device.ip);
				crequest.device.location = location;
			}
		}

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
	
		if ((label = json_find_first_label(temp, "make")) != NULL)
		{
			string s_make = label->child->text;
			if(s_make != "")
			{	
				map<string, uint16_t>::iterator it;

				crequest.device.makestr = s_make;
				//transform(s_make.begin(), s_make.end(), s_make.begin(), ::tolower);
				for (it = dev_make_table.begin();it != dev_make_table.end();++it)
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
			if(strcasecmp(label->child->text, "iPhone OS") == 0 || strcasecmp(label->child->text, "iOS") == 0)
			{
				crequest.device.os = 1;
				dpidtype = DPID_IDFA;
				request.device.os = 1;
			}
			else if(strcasecmp(label->child->text, "Android") == 0)
			{
				crequest.device.os = 2;
				dpidtype = DPID_ANDROIDID;
				request.device.os = 2;
				
			}
			else if(strcasecmp(label->child->text, "Windows") == 0)
			{
				crequest.device.os = 3;
				dpidtype = DPID_OTHER;
				request.device.os = 3;
			}

			if(dpid.size() > 0)
				crequest.device.dpids.insert(make_pair(dpidtype, dpid));
		}

		if(crequest.device.dids.empty() && crequest.device.dpids.empty())
		{
			err = E_INVALID_PARAM;
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
				crequest.device.devicetype = DEVICE_PHONE;
				break;
			case 3:
			case 5:
				crequest.device.devicetype = DEVICE_TABLET;
				break;
			case 6:
				crequest.device.devicetype = DEVICE_TV;
				break;
			default:
				crequest.device.devicetype = DEVICE_UNKNOWN;
				break;
			}
		}

		if ((label = json_find_first_label(temp, "loc")) != NULL)
		{
			char *p = NULL;
			char *outer_ptr=NULL;  
			request.device.loc = label->child->text;
			//sscanf(label->child->text, "%lf,%lf", &crequest.device.geo.lat, &crequest.device.geo.lon);
			p = strtok_r(label->child->text, ",", &outer_ptr);
			if(p)
			{
				crequest.device.geo.lat = atof(p);
				//cout <<"lat :" << crequest.device.geo.lat <<endl;
				p=strtok_r(NULL, ",", &outer_ptr);
				if(p)
				{
					crequest.device.geo.lon = atof(p);
					//cout <<"lon :" << crequest.device.geo.lon <<endl;
				}
			}
		}

		if ((label = json_find_first_label(temp, "density")) != NULL)
			request.device.density = atof(label->child->text);
		if ((label = json_find_first_label(temp, "sw")) != NULL
			&& label->child->type == JSON_NUMBER)
			request.device.sw = atoi(label->child->text);
		if ((label = json_find_first_label(temp, "sh")) != NULL
			&& label->child->type == JSON_NUMBER)
		{
			request.device.sh = atoi(label->child->text);
		}

		if ((label = json_find_first_label(temp, "orientation")) != NULL
			&& label->child->type == JSON_NUMBER)
			request.device.orientation = atoi(label->child->text);

		if(request.imps.size() && (err == E_SUCCESS))
		{
			char size[16] = "";
			string ads_size;
			int i, j, index, instl;
			bool enlarged;
			index = instl = 0;

			if(request.imps[0].instl)
			{
				//设定request id的生存期为120分钟
				crequest.imp[0].requestidlife = 7200;
				//重新设置创意的宽高
				if (request.device.orientation == 1)
					sprintf(size, "%dx%d", request.device.sw, request.device.sh);
				else
					sprintf(size, "%dx%d", request.device.sh, request.device.sw);
			}
			else
			{
				crequest.imp[0].requestidlife = 600;
				sprintf(size, "%dx%d", crequest.imp[0].banner.w, crequest.imp[0].banner.h);
			}

			ads_size = size;

			for (i = 0; i < ADS_AMOUNT; ++i)
			{
				if (g_ads_size[i] == ads_size)
				{
					if (request.app.bundle == "com.esvideo")
					{
						writeLog(g_logid_local, LOGINFO, "app: com.esvideo, request: fullscreen, status: refused");
						err =  E_INVALID_PARAM;
					}
					index = i;
					instl = 2;
					break;
				}
			}

			if (i == ADS_AMOUNT)
			{
				if (request.imps[0].instl)
				{
					int sizeratio;
					if(request.device.sw != 0 && request.device.sh != 0)
					{
						if (request.device.orientation == 1)
							sizeratio = request.device.sw * 10000 / request.device.sh;
						else
							sizeratio = request.device.sh * 10000 / request.device.sw;

						for (j = 0; j < ADS_AMOUNT; ++j)
						{
							if (g_ads_sizeratio[j] > sizeratio)
							{
								if (j == 0)
								{
									strcpy(size, g_ads_size[g_ads_sizeratio_upgrade_index[j]].c_str());
									instl = 1;
									break;
								}
								else
								{
									if (sizeratio - g_ads_sizeratio[j - 1] > g_ads_sizeratio[j] - sizeratio)
									{
										strcpy(size, g_ads_size[g_ads_sizeratio_upgrade_index[j]].c_str());
										instl = 1;
										break;
									}
									else
									{
										strcpy(size, g_ads_size[g_ads_sizeratio_upgrade_index[j - 1]].c_str());
										instl = 1;
										break;
									}
								}
							}
						}
					}
					else
					{
						err = E_INVALID_PARAM;
					}
				}
			}
			else
			{
				//横幅需要根据density来扩大分辨率
				if(request.imps[0].instl == 0)
				{
					bool enlarged = false;

					switch (request.device.devicetype)
					{
					case 1: case 3:         // iOS
						if (request.device.density >= 2)
							enlarged = true;
						break;
					case 2: case 5:         // Android
						if (request.device.density >= 1.5)
							enlarged = true;
						break;
					}
					//当density为高清时
				if (enlarged && index < 8 && g_ads_enlarge[index])
						sprintf(size, "%dx%d", crequest.imp[0].banner.w * 2, crequest.imp[0].banner.h * 2);	
				}
			}

			request.instl = instl;
			if(request.imps[0].instl)
				crequest.imp[0].ext.instl = instl;

			sscanf(size, "%dx%d", &crequest.imp[0].banner.w, &crequest.imp[0].banner.h);
			/*char *p = NULL;
			char *outer_ptr=NULL;  
			p = strtok_r(size, "x", &outer_ptr);
			if(p)
			{
				crequest.imp[0].banner.w = atoi(p);
				p=strtok_r(NULL, "x", &outer_ptr);
				if(p)
				{
					crequest.imp[0].banner.h = atoi(p);	
				}
			}*/
		}
		//writeLog(g_logid_local, LOGDEBUG, "impinstal : %d and reqinstl: %d and wxh: %dx%d", request.imps[0].instl, request.instl,  crequest.imp[0].banner.w, crequest.imp[0].banner.h);
	}
	else
	{
		err = E_INVALID_PARAM;
	}

	crequest.cur.insert("CNY");

	if ((label = json_find_first_label(root, "bcat")) != NULL &&label->child != NULL)
	{
		//需通过映射,传递给后端
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.bcat.insert(temp->text);
			vector<uint32_t> &v_cat = adv_cat_table_adxi[temp->text];
			for(uint32_t i = 0; i < v_cat.size(); ++i)
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
	
exit:
	if(root != NULL)
		json_free_value(&root);
	return err;
}

static void setAmaxJsonResponse(COM_REQUEST &request, COM_RESPONSE &cresponse, ADXTEMPLATE &adxtemp, string &data_out, int errcode)
{
	char *text = NULL;
	json_t *root, *label_seatbid, *label_bid, *label_ext,
		*array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;

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
					jsonInsertInt(entry_bid, "price", bid.price *10000);
					jsonInsertString(entry_bid, "adid", bid.adid.c_str());

					if (adxtemp.nurl.size() > 0)
					{
						string nurl = adxtemp.nurl;
						replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, nurl);
						jsonInsertString(entry_bid, "nurl", nurl.c_str());
					}
					//
					if (adxtemp.adms.size() > 0)
					{
						string adm = "";
						//根据os和type查找对应的adm
						for(int count = 0; count < adxtemp.adms.size(); ++count)
						{
							if(adxtemp.adms[count].os == request.device.os && adxtemp.adms[count].type == bid.type)
							{
								adm = adxtemp.adms[count].adm;
								break;
							}
						}

						if(adm.size() > 0)
						{
							replaceMacro(adm, "#SOURCEURL#", bid.sourceurl.c_str());
							//IURL需要替换
							string iurl;
							if(adxtemp.iurl.size() > 0)
							{
								iurl = adxtemp.iurl[0];
								replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, iurl);
							}
							for(int i =1; i <bid.imonitorurl.size(); ++i)
							{
								if(bid.imonitorurl[i].find("admaster") != string::npos)
								{
									replaceAdmasterMonitor(hash_ctx, request.device, bid.imonitorurl[i], request.id);
								}
								else if(bid.imonitorurl[i].find("nielsen") != string::npos)
								{
									replaceNielsenMonitor(hash_ctx, request.device, bid.imonitorurl[i]);
								}
								iurl += "'/><img style='width:0;height:0' src='" + bid.imonitorurl[i];
							}
							replaceMacro(adm, "#IURL#", iurl.c_str());
							jsonInsertString(entry_bid, "adm", adm.c_str());
						}
						
					}
					if (request.imp[0].ext.instl  && bid.w > 0)
						jsonInsertInt(entry_bid, "adw", bid.w);
					if (request.imp[0].ext.instl && bid.h > 0)
						jsonInsertInt(entry_bid, "adh", bid.h);

					if (bid.imonitorurl.size() > 0)
					{
						string imonitorurl = bid.imonitorurl[0];
						if(imonitorurl.find("admaster") != string::npos)
						{
							replaceAdmasterMonitor(hash_ctx, request.device, imonitorurl, request.id);
						}
						else if(imonitorurl.find("nielsen") != string::npos)
						{
							replaceNielsenMonitor(hash_ctx, request.device, imonitorurl);
						}
						jsonInsertString(entry_bid, "iurl", imonitorurl.c_str());
					}
					
					string curl = bid.curl;
					if(curl.find("admaster") != string::npos)
					{
						replaceAdmasterMonitor(hash_ctx, request.device, curl, request.id);
					}
					else if(curl.find("nielsen") != string::npos)
					{
						replaceNielsenMonitor(hash_ctx, request.device, curl);
					}
					if(bid.effectmonitor == 1)
					{
						replaceCurlParam(request.id, request.device, bid.mapid, curl);
					}
					
					int k = 0;
					//根据redirect判断是否需要拼接desturl
					if(bid.redirect)
					{
						if (adxtemp.cturl.size() > 0)
						{
							int lenout;
							string strtemp = curl;
							replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, strtemp);
							char *enurl = urlencode(strtemp.c_str(), strtemp.size(),  &lenout);
							curl = adxtemp.cturl[0];
							++k;
							curl += "&curl=" + string(enurl, lenout);
							free(enurl);
						}
					}
					//curl需替换BID宏MAPID宏
					replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, curl);

					jsonInsertString(entry_bid, "curl", curl.c_str());

					if (1)
					{
						json_t *label_cturl, *array_cturl, *value_cturl;

						label_cturl = json_new_string("cturl");
						array_cturl = json_new_array();

						if(k < adxtemp.cturl.size())
						{
							string cturl = adxtemp.cturl[k];
							cturl += "&curl=";
							replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, cturl);
							value_cturl = json_new_string(cturl.c_str());
							json_insert_child(array_cturl, value_cturl);
							++k;
						}

						for (; k < adxtemp.cturl.size(); ++k)
						{
							string cturl = adxtemp.cturl[k];
							replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, cturl);
							value_cturl = json_new_string(cturl.c_str());
							json_insert_child(array_cturl, value_cturl);
						}
						for(k = 0; k < bid.cmonitorurl.size(); ++k)
						{
							if(bid.cmonitorurl[k].find("admaster") != string::npos)
							{
								replaceAdmasterMonitor(hash_ctx, request.device, bid.cmonitorurl[k], request.id);
							}
							else if(bid.cmonitorurl[k].find("nielsen") != string::npos)
							{
								replaceNielsenMonitor(hash_ctx, request.device, bid.cmonitorurl[k]);
							}
							value_cturl = json_new_string(bid.cmonitorurl[k].c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						json_insert_child(label_cturl, array_cturl);
						json_insert_child(entry_bid, label_cturl);
					}

					if (!adxtemp.aurl.empty() && adxtemp.aurl.size() > 0)
					{
						string aurl;
						aurl = adxtemp.aurl;
						replace_url(request.id, bid.mapid, request.device.deviceid, request.device.deviceidtype, aurl);
						jsonInsertString(entry_bid, "aurl", aurl.c_str());
					}

					if (bid.cid.size() > 0)
						jsonInsertString(entry_bid, "cid", bid.cid.c_str());
					if (bid.crid.size() > 0)
						jsonInsertString(entry_bid, "crid", bid.crid.c_str());
					//ctype需转换
					int ctype = CTYPE_UNKNOWN;
					switch(bid.ctype)
					{
					case CTYPE_OPEN_WEBPAGE:
						ctype = 3;
						break;
					case CTYPE_DOWNLOAD_APP:
						ctype = 2;
						break;
					case CTYPE_DOWNLOAD_APP_FROM_APPSTORE:
						ctype = 6;
						break;
					case CTYPE_SENDSHORTMSG:
						ctype = 4;
						break;
					case CTYPE_CALL:
						ctype = 5;
						break;
					default:
						ctype = 10;
					}
					jsonInsertInt(entry_bid, "ctype", ctype);
					/*
					if (bid->ctype > 0)
						jsonInsertInt(entry_bid, "ctype", bid->ctype);
						*/
					if (bid.ctype == CTYPE_DOWNLOAD_APP &&  bid.bundle.size() > 0)
						jsonInsertString(entry_bid, "cbundle", bid.bundle.c_str());

					if (bid.attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						set<uint8_t>::iterator it; //定义前向迭代器 遍历集合中的所有元素  
					
						for(it = bid.attr.begin(); it != bid.attr.end(); ++it) 				
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
					if (bid.adomain.size() > 0)
						jsonInsertString(entry_bid, "adomain", bid.adomain.c_str());

					//if (bid->ext.size() >  0)
					{
						json_t *label_ext, *entry_ext;
						label_ext = json_new_string("ext");
						entry_ext = json_new_object();
						if (request.imp[0].ext.instl)
						{
							//需考虑谁来提供，adapter判断是全屏还是插屏
							jsonInsertInt(entry_ext, "instl", request.imp[0].ext.instl);
						}

						//开屏在parse时过滤掉了
						/*if(request.splash)
						{
							//需考虑从哪儿获取开屏返回的信息
							jsonInsertInt(entry_ext, "adt", bid->exts[0].adt);
							
						}*/
						if (bid.ctype == CTYPE_DOWNLOAD_APP)
						{
							/*json_t *tmproot = NULL;
							json_t *tmplabel;
							map<uint8_t, string >::const_iterator ext_it  = bid->exts.find(ADX_AMAX);
							if(ext_it != bid->exts.end())
							{
								string extinfo = ext_it->second;
								json_parse_document(&tmproot, extinfo.c_str());

								if (tmproot != NULL)
								{
									if(tmplabel = json_find_first_label(tmproot, "apkname"))
										if(strlen(tmplabel->child->text) != 0)
											jsonInsertString(entry_ext, "apkname", tmplabel->child->text);
									json_free_value(&tmproot);
								}
							}*/
							if(bid.apkname.size() > 0)
								jsonInsertString(entry_ext, "apkname", bid.apkname.c_str());
						}
						json_insert_child(label_ext, entry_ext);
						json_insert_child(entry_bid, label_ext);
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
	if(text != NULL)
	{
		data_out = text;
		free(text);
		text = NULL;
	}

	if(root != NULL)
		json_free_value(&root);

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "End setAmaxJsonResponse");
#endif
	return;
}

void transform_request(MESSAGEREQUEST &mrequest, COM_REQUEST &crequest)
{

}

void transform_response(COM_RESPONSE &cresponse, MESSAGERESPONSE &mresponse)
{

}

static bool filter_cbidobject_callback(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price)
{
	if(price - crequest.imp[0].bidfloor  - 0.01 > 0.000001)
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

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata,  OUT string &senddata)
{
	int err = E_SUCCESS;
	FULL_ERRCODE ferrcode = {0};
	//RECVDATA *recvdata = arg;
	uint32_t sendlength = 0;
	MESSAGEREQUEST mrequest;//adx msg request
	COM_REQUEST crequest;//common msg request
	string output = "";
	int ret;
	COM_RESPONSE cresponse;
	string outputdata;//, senddata;
	ADXTEMPLATE adxtemp;
	struct timespec ts1, ts2;
	long lasttime;
	string str_ferrcode = "";

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
	getTime(&ts1);
	init_message_request(mrequest);
	init_com_message_request(&crequest);

	err = parseAmaxRequest(recvdata->data, mrequest, crequest);
	if(err == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);
		goto exit;
	}
#ifdef DEBUG
	print_com_request(crequest);
#endif
	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s, getRegionCode ip:%s location:%d failed!", crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		err = E_REQUEST_DEVICE_IP;
	}
	else if (err == E_SUCCESS)
	{
		init_com_message_response(&cresponse);
		init_ADXTEMPLATE(adxtemp);
		ferrcode = bid_filter_response(ctx, index, crequest, NULL, filter_cbidobject_callback, NULL, &cresponse, &adxtemp);	
		err = ferrcode.errcode;
		str_ferrcode = fullerrcode_to_string(ferrcode, ferrcode.level);
#ifdef DEBUG
		if(err == E_SUCCESS)
		print_com_response(cresponse);
#endif
		//cflog(g_logid_local, LOGDEBUG, (char*)(string(crequest.id + ": " + str_ferrcode).c_str()));
	}

	setAmaxJsonResponse(crequest, cresponse, adxtemp, output, err);
	
	senddata = "Content-Type: application/json\r\nx-amaxrtb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

	sendLog(datatransfer, crequest, ADX_AMAX, err);
	if(err == E_SUCCESS)
	{
		numcount[index]++;
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d", cresponse.id.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(), crequest.device.deviceid.c_str(), crequest.device.deviceidtype);
		if(numcount[index] %1000 == 0)
		{
			writeLog(g_logid_local, LOGDEBUG, "response :" + output);
			numcount[index] = 0;
		}
	}
	//else if(fullreqrecord)
	//		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	//writeLog(g_logid_local, LOGDEBUG, "%s,spent time:%ld,err:0x%x,tid:0x%x,desc:%s", mrequest.id.c_str(), lasttime, err, datatransfer->threadlog,str_ferrcode.c_str());
	writeLog(g_logid_local, LOGDEBUG, "%s, spent time:%ld,err:0x%x,desc:%s", mrequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x, tid:0x%x",  mrequest.id.c_str(), lasttime, err, datatransfer->threadlog);

	if(err != E_REDIS_CONNECT_INVALID)
		err = E_SUCCESS;
/*back:
	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);*/

exit:
	return err;
}




