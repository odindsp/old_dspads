/*
 * adwalker_response.cpp
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
#include "adwalker_response.h"
#include "../../common/bid_filter.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"

#define ADS_AMOUNT 20
extern map<string, vector<uint32_t> > app_cat_table;
extern map<string, vector<uint32_t> > adv_cat_table_adxi;
extern map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
extern int *numcount;
extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local, g_logid_response;
extern MD5_CTX hash_ctx; 

char *uintToStr(int arg)
{
	char num[16];

	sprintf(num, "%u", arg);

	return num;
}

void replace_url(IN COM_REQUEST crequest, IN string mapid, INOUT string &url)
{
	replaceMacro(url, "#BID#", crequest.id.c_str());
	replaceMacro(url, "#MAPID#", mapid.c_str());
	replaceMacro(url, "#DEVICEID#", crequest.device.deviceid.c_str());
	replaceMacro(url, "#DEVICEIDTYPE#", intToString(crequest.device.deviceidtype).c_str());
}

template<typename T>
void jsonGetIntegerSet(json_t *label, set<T> &v)
{
	if (label == NULL || label->child == NULL || label->child->type != JSON_ARRAY)
		return;

	json_t *temp = label->child->child;

	if (temp == NULL || temp->type != JSON_NUMBER)
		return;

	while (temp != NULL)
	{
		v.insert(atoi(temp->text));
		temp = temp->next;
	}
}

uint64_t iab_to_uint64(string cat)
{
	int icat = 0;
	sscanf(cat.c_str(), "%d", &icat);
	return icat;
}

void printfvecstring(vector<string> &v)
{
	int i = 0;
	for(; i < v.size(); i++)
		cout << v[i] << endl;
}

void printfsetstring(set<string> &v)
{
	set<string>::iterator it;
	for(it=v.begin();it!=v.end();it++) 
		cout<<*it <<endl;   
}

void printfvectorInt8(vector<uint8_t> &v)
{
	int i = 0;
	for(; i < v.size(); i++)
		cout << (int)v[i] << endl;
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

int parseadwalkerRequest(char *data, COM_REQUEST &crequest)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;

	if (data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseadwalkerRequest root is NULL!");
		return E_BAD_REQUEST;
	}
	
	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		crequest.id = label->child->text;
	}
	else
	{
		err =  E_BAD_REQUEST;
		goto exit;
	}

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *value_imp_detail = NULL;
		json_t *value_imp = label->child->child;

		if (value_imp != NULL)
		{
			COM_IMPRESSIONOBJECT cimp;

			init_com_impression_object(&cimp);
			
			if ((label = json_find_first_label(value_imp, "id")) != NULL)
				cimp.id = label->child->text;

			if ((label = json_find_first_label(value_imp, "bidfloor")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				cimp.bidfloor = atoi(label->child->text) /10000.0;
			}

			/*if ((label = json_find_first_label(value_imp, "bidfloorcur")) != NULL)
				imp.bidfloorcur = label->child->text;*/

			if((label = json_find_first_label(value_imp, "banner")) != NULL)
			{
				cimp.type = IMP_BANNER;
				
				value_imp_detail = label->child;
				if ((label = json_find_first_label(value_imp_detail, "w")) != NULL
					&& label->child->type == JSON_NUMBER)
					cimp.banner.w = atoi(label->child->text);

				if ((label = json_find_first_label(value_imp_detail, "h")) != NULL
					&& label->child->type == JSON_NUMBER)
					cimp.banner.h = atoi(label->child->text);

				if ((label = json_find_first_label(value_imp_detail, "btype")) != NULL)
				{
					json_t *temp = label->child->child;
					//需要转换映射，待讨论
					while (temp != NULL)
					{
						cimp.banner.btype.insert(atoi(temp->text));
						temp = temp->next;
					}
				}

				if ((label = json_find_first_label(value_imp_detail, "battr")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						cimp.banner.battr.insert(atoi(temp->text));
						temp = temp->next;
					}
				}
			}
			else if((label = json_find_first_label(value_imp, "native")) != NULL && label->child->type == JSON_OBJECT)
			{//now not support
				err = E_INVALID_PARAM;
			}

			if ((label = json_find_first_label(value_imp, "instl")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				cimp.ext.instl = atoi(label->child->text);
				if(cimp.ext.instl != 0)
					cimp.requestidlife = 7200;
			}

			if((label = json_find_first_label(value_imp, "ext")) != NULL)
			{
				json_t *tmp = label->child;
				if ((label = json_find_first_label(tmp, "splash")) != NULL
					&& label->child->type == JSON_NUMBER)
				{
					cimp.ext.splash = atoi(label->child->text);
					if(cimp.ext.splash)
						err = E_INVALID_PARAM;
				}
			}
			crequest.imp.push_back(cimp);
		}
	}
	else
	{
		err = E_INVALID_PARAM;
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "id")) != NULL)
		{
			crequest.app.id = label->child->text;
			if(crequest.app.id == "4101856e6dba4778a73b0cbe0cb5dad9")
				err = E_INVALID_PARAM;
		}
		if ((label = json_find_first_label(temp, "name")) != NULL)
			crequest.app.name = label->child->text;
		if ((label = json_find_first_label(temp, "cat")) != NULL)
		{
			//待映射
			json_t *temp = label->child->child;
			while (temp != NULL)
			{
				int  cat = atoi(temp->text);
				if(cat == 60001 || cat ==  60501)
				{
					err = E_INVALID_PARAM;
					//writeLog(g_logid_local, LOGDEBUG, "invalid cat ios book");
					//日志服务器需要所有的cat种类，不应该break
					//break;
				}

				vector<uint32_t> &v_cat = app_cat_table[temp->text];
				for (uint32_t i = 0; i < v_cat.size(); i++)
					crequest.app.cat.insert(v_cat[i]);

				temp = temp->next;
			}
			//驾校一点通的cat中60001或60501
			if(temp != NULL && strcmp(crequest.app.id.c_str(), "bf90666fc027407bb847050f4a3dbfb0") == 0)
				err = E_SUCCESS;
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
		string dpid, did;
		uint8_t dpidtype = DPID_UNKNOWN;

		if ((label = json_find_first_label(temp, "did")) != NULL)
		{
			did = label->child->text;
			//request.device.did = did;
			transform(did.begin(), did.end(), did.begin(), ::tolower);
			crequest.device.dids.insert(make_pair(DID_IMEI_SHA1, did));
		}
		
		if ((label = json_find_first_label(temp, "dpid")) != NULL)
		{
			dpid = label->child->text;
			//request.device.dpid = label->child->text;
		}

		if ((label = json_find_first_label(temp, "mac")) != NULL)
		{
			string mac = label->child->text;
			transform(mac.begin(), mac.end(), mac.begin(), ::tolower);
			crequest.device.dids.insert(make_pair(DID_MAC_SHA1, mac));
		}
		
		if ((label = json_find_first_label(temp, "ua")) != NULL)
			crequest.device.ua = label->child->text;

		if((label = json_find_first_label(temp, "geo")) != NULL && label->child->type == JSON_OBJECT)
		{
			json_t *jgeo = label->child;
			if((label = json_find_first_label(jgeo, "lat")) != NULL)
			{
				crequest.device.geo.lat = atof(label->child->text);
			}
			if((label = json_find_first_label(jgeo, "lon")) != NULL)
			{
				crequest.device.geo.lon = atof(label->child->text);
			}
		}

		if ((label = json_find_first_label(temp, "ip")) != NULL)
			crequest.device.ip = label->child->text;
	
		if ((label = json_find_first_label(temp, "carrier")) != NULL)
		{
			int carrier = CARRIER_UNKNOWN;
			carrier = atoi(label->child->text);
			if(carrier == 46000 || carrier == 46002 || carrier == 46007)
				crequest.device.carrier = CARRIER_CHINAMOBILE;
			else if(carrier == 46001 || carrier == 46006)
				crequest.device.carrier = CARRIER_CHINAUNICOM;
			else if(carrier == 46003 || carrier == 46005)
				crequest.device.carrier = CARRIER_CHINATELECOM;

		}
	
		if ((label = json_find_first_label(temp, "make")) != NULL)
		{
			writeLog(g_logid_local, LOGDEBUG, "%s,not find make:%s", crequest.id.c_str(), temp->child->text);
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
			}
			else if(strcasecmp(label->child->text, "Android") == 0)
			{
				crequest.device.os = 2;
				dpidtype = DPID_ANDROIDID;				
			}
			else if(strcasecmp(label->child->text, "Windows") == 0)
			{
				crequest.device.os = 3;
				dpidtype = DPID_OTHER;
			}

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
			int devicetype = atoi(label->child->text);
			switch(devicetype)
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
			vector<uint32_t> &v_cat = adv_cat_table_adxi[temp->text];
			for(uint32_t i = 0; i < v_cat.size(); i++)
				crequest.bcat.insert(v_cat[i]);

			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "wcat")) != NULL &&label->child != NULL)
	{
		//需通过映射,传递给后端
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			vector<uint32_t> &v_cat = adv_cat_table_adxi[temp->text];
			for(uint32_t i = 0; i < v_cat.size(); i++)
				crequest.bcat.insert(v_cat[i]);

			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL &&label->child != NULL)
		jsonGetStringSet(label, crequest.badv);
		
exit:
	if(root != NULL)
		json_free_value(&root);
	return err;
}

static void setadwalkerJsonResponse(COM_REQUEST &request, COM_RESPONSE &cresponse, ADXTEMPLATE &adxtemp, string &data_out, int errcode)
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
		for (i = 0; i < cresponse.seatbid.size(); i++)
		{
			COM_SEATBIDOBJECT *seatbid = &cresponse.seatbid[i];
			entry_seatbid = json_new_object();
			if (!seatbid->bid.empty() && seatbid->bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();
				for (j = 0; j < seatbid->bid.size(); j++)
				{
					COM_BIDOBJECT *bid = &seatbid->bid[j];
					entry_bid = json_new_object();
					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					jsonInsertInt(entry_bid, "price", bid->price *10000);
					jsonInsertString(entry_bid, "adid", bid->adid.c_str());

					/*if (!adxtemp.nurl.empty() && adxtemp.nurl.size() > 0)
					{
						string nurl = adxtemp.nurl;
						replace_url(request, bid->mapid, nurl);
						jsonInsertString(entry_bid, "nurl", nurl.c_str());
					}*/
					//adm，广告物料，Banner广告为Banner对象，Native广告为Native对象
					if(1)
					{
						json_t *adm_obj = json_new_object();
						json_t *adm_label = json_new_string("adm");
						uint8_t imptype = request.imp[0].type;
						if(imptype == IMP_BANNER)
						{
							string img;
							string txt;
							string desc;
							string html_url;

							uint8_t crt_type;
							
							switch(bid->type)
							{
							case 1:
								{
									jsonInsertString(adm_obj, "txt", txt.c_str());
									jsonInsertString(adm_obj, "desc", desc.c_str());
								}
								break;
							case ADTYPE_IMAGE:
								{
									crt_type = 2;
									jsonInsertString(adm_obj, "img", bid->sourceurl.c_str());
								}
								break;
							case 3:
								{
									jsonInsertString(adm_obj, "img", bid->sourceurl.c_str());
									jsonInsertString(adm_obj, "txt", txt.c_str());
									jsonInsertString(adm_obj, "desc", desc.c_str());
								}
								break;
							case 4:
								{
									crt_type = 4;
									string adm = "";
									if (!adxtemp.adms.empty() && adxtemp.adms.size() > 0)
									{
										//根据os和type查找对应的adm
										for(int count = 0; count < adxtemp.adms.size(); count++)
										{
											if(adxtemp.adms[count].os == request.device.os && adxtemp.adms[count].type == bid->type)
											{
												adm = adxtemp.adms[count].adm;
												break;
											}
										}

										if(adm.size() > 0)
										{
											replaceMacro(adm, "#SOURCEURL#", bid->sourceurl.c_str());
											//IURL需要替换
											string iurl;
											if(adxtemp.iurl.size() > 0)
											{
												iurl = adxtemp.iurl[0];
												replace_url(request, bid->mapid, iurl);
											}

											replaceMacro(adm, "#IURL#", iurl.c_str());
										}

									}
									jsonInsertString(adm_obj, "html", adm.c_str());
								}
								
								break;
							case 5:
								jsonInsertString(adm_obj, "html_url", html_url.c_str());
								break;
							}
							jsonInsertInt(adm_obj, "crt_type", crt_type);
						}
						else if (imptype == IMP_NATIVE)
						{

						}
						json_insert_child(adm_label, adm_obj);
						json_insert_child(entry_bid, adm_label);
					}
					
					if (request.imp[0].ext.instl  && bid->w > 0)
						jsonInsertInt(entry_bid, "w", bid->w);
					if (request.imp[0].ext.instl && bid->h > 0)
						jsonInsertInt(entry_bid, "h", bid->h);

					//iurl为数组
					json_t *label_iurl, *array_iurl, *value_iurl;
					label_iurl = json_new_string("iurl");
					array_iurl = json_new_array();

					if(bid->type == ADTYPE_IMAGE)//当是html时，已经使用过iurl[0]
					{
						if(adxtemp.iurl.size() > 0)
						{
							string iurl = adxtemp.iurl[0] + "&url=";
							replace_url(request, bid->mapid, iurl);
							value_iurl = json_new_string(iurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}
					}
					
					if (bid->imonitorurl.size() > 0)
					{
						for(int i = 0; i <bid->imonitorurl.size(); ++i)
						{
							string imonitorurl = bid->imonitorurl[i];
							if(imonitorurl.find("__OS__") < imonitorurl.size())
							{
								replaceThirdMonitor(hash_ctx, request.device, imonitorurl);
							}
							value_iurl = json_new_string(imonitorurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}
					}
					json_insert_child(label_iurl, array_iurl);
					json_insert_child(entry_bid, label_iurl);

					string curl = bid->curl;
					if(curl.find("__OS__") < curl.size())
					{
						replaceThirdMonitor(hash_ctx, request.device, curl);
					}

					int k = 0;
					//根据redirect判断是否需要拼接desturl
					if(bid->redirect)
					{
						if (!adxtemp.cturl.empty() && adxtemp.cturl.size() > 0)
						{
							int lenout;
							string strtemp = curl;
							replace_url(request, bid->mapid, strtemp);
							char *enurl = urlencode(strtemp.c_str(), strtemp.size(),  &lenout);
							curl = adxtemp.cturl[0];
							k++;
							curl += "&curl=" + string(enurl, lenout);
							free(enurl);
						}
					}
					//curl需替换BID宏
					replace_url(request, bid->mapid, curl);

					if (bid->cid.length() > 0)
						jsonInsertString(entry_bid, "cid", bid->cid.c_str());
					if (bid->crid.length() > 0)
						jsonInsertString(entry_bid, "crid", bid->crid.c_str());
					if (bid->ctype == 2 &&  bid->bundle.length() > 0)
						jsonInsertString(entry_bid, "bundle", bid->bundle.c_str());

					if (bid->attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						set<uint8_t>::iterator it; //定义前向迭代器 遍历集合中的所有元素  
					
						for(it = bid->attr.begin();it != bid->attr.end();it++) 				
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
					if (bid->adomain.length() > 0)
						jsonInsertString(entry_bid, "adomain", bid->adomain.c_str());
					//ext对象
					//if (bid->ext.size() >  0)
					{
						json_t *label_ext, *entry_ext, *link, *link_label, *link_ext, *link_ext_label;
						int ctype = 0;
						label_ext = json_new_string("ext");
						entry_ext = json_new_object();
						
						link_label = json_new_string("link");
						link = json_new_object();

						link_ext_label = json_new_string("ext");
						link_ext = json_new_object();

						jsonInsertString(link, "url", curl.c_str());
						
						//clicktracker
						{
							json_t *label_cturl, *array_cturl, *value_cturl;

							label_cturl = json_new_string("clicktracker");
							array_cturl = json_new_array();

							if(k < adxtemp.cturl.size())
							{
								string cturl = adxtemp.cturl[k];
								cturl += "&url=";
								replace_url(request, bid->mapid, cturl);
								value_cturl = json_new_string(cturl.c_str());
								json_insert_child(array_cturl, value_cturl);
								k++;
							}

							for (; k < adxtemp.cturl.size(); k++)
							{
								string cturl = adxtemp.cturl[k];
								replace_url(request, bid->mapid, cturl);
								value_cturl = json_new_string(cturl.c_str());
								json_insert_child(array_cturl, value_cturl);
							}
							for(k = 0; k < bid->cmonitorurl.size(); ++k)
							{
								string cmonitorurl = bid->imonitorurl[i];
								if(cmonitorurl.find("__OS__") < cmonitorurl.size())
								{
									replaceThirdMonitor(hash_ctx, request.device, cmonitorurl);
								}
								value_cturl = json_new_string(cmonitorurl.c_str());
								json_insert_child(array_cturl, value_cturl);
							}

							json_insert_child(label_cturl, array_cturl);
							json_insert_child(link, label_cturl);
						}

						switch(bid->ctype)
						{
						case 1:
							ctype = 3;
							break;
						case 2:
							ctype = 2;
							break;
						case 3:
							ctype = 6;
							break;
						case 4:
							ctype = 4;
							break;
						case 5:
							ctype = 5;
							break;
						default:
							ctype = 10;
						}
						jsonInsertInt(link_ext, "type", ctype);
						json_insert_child(link_ext_label, link_ext);

						json_insert_child(link, link_ext_label);

						json_insert_child(link_label, link);
						json_insert_child(entry_ext, link_label);

						if (request.imp[0].ext.instl)
						{
							//需考虑谁来提供，adapter判断是全屏还是插屏
							jsonInsertInt(entry_ext, "instl", request.imp[0].ext.instl);
						}

						if(request.imp[0].ext.splash)
						{
							//需考虑从哪儿获取开屏返回的信息
							json_t *tmproot = NULL;
							json_t *tmplabel;
							string extinfo = bid->creative_ext.ext;
							if(extinfo.size() > 0)
							{
								json_parse_document(&tmproot, extinfo.c_str());

								if (tmproot != NULL)
								{
									if((tmplabel = json_find_first_label(tmproot, "adt")) != NULL && tmplabel->child->type == JSON_NUMBER)
											jsonInsertInt(entry_ext, "adt", atoi(tmplabel->child->text));

									if(tmplabel = json_find_first_label(tmproot, "ade"))
										if(strlen(tmplabel->child->text) != 0)
											jsonInsertString(entry_ext, "ade", tmplabel->child->text);

									json_free_value(&tmproot);
								}
							}
							//jsonInsertInt(entry_ext, "adt", bid->exts[0].adt);
							
						}
						if (bid->ctype == 2)
						{
							
							if(bid->apkname.length() > 0)
								jsonInsertString(entry_ext, "apkname", bid->apkname.c_str());
						}

						if (adxtemp.aurl.length() > 0)
						{
							string aurl;
							aurl = adxtemp.aurl;
							replace_url(request, bid->mapid, aurl);
							jsonInsertString(entry_ext, "aurl", aurl.c_str());
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
	json_free_value(&root);
	return;
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

	for (int i = 0; i < crequest.imp.size(); i++ )
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];
		cimp.bidfloor /= ratio;
	}

	return true;
}

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata)
{
	int err = E_SUCCESS;
	//RECVDATA *recvdata = arg;
	COM_REQUEST crequest;//common msg request
	COM_RESPONSE cresponse;
	ADXTEMPLATE adxtemp;
	string output = "";
	struct timespec ts1, ts2;
	long lasttime;
	FULL_ERRCODE ferrcode = {0};
	string str_ferrcode = "";

	if (recvdata == NULL)
	{
		err = E_BAD_REQUEST;
		goto exit;
	}

	if ((recvdata->data == NULL)||(recvdata->length == 0))
	{
		err = E_BAD_REQUEST;
		goto exit;
	}
	getTime(&ts1);
	init_com_message_request(&crequest);

	err = parseadwalkerRequest(recvdata->data, crequest);
	if(err == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGDEBUG, "recvdata: %s", recvdata->data);
		goto exit;
	}
	if (err == E_SUCCESS)
	{
		init_com_message_response(&cresponse);

		ferrcode = bid_filter_response(ctx, index, crequest, NULL, filter_cbidobject_callback, NULL, &cresponse, &adxtemp);	
		
		err = ferrcode.errcode;
		str_ferrcode = fullerrcode_to_string(ferrcode, ferrcode.level);
	}

	setadwalkerJsonResponse(crequest, cresponse, adxtemp, output, err);
	
	senddata = "Content-Type: application/json\r\nx-adwalkerrtb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

	sendLog(datatransfer, crequest, ADX_ADWALKER, err);
	if(err == 0)
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
	writeLog(g_logid_local, LOGDEBUG, "%s,spent time:%ld,err:0x%x,desc:%s", crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s, spent time:%ld, err:0x%x, tid:0x%x",  crequest.id.c_str(), lasttime, err, datatransfer->threadlog);

	err = E_SUCCESS;

exit:
	return err;
}