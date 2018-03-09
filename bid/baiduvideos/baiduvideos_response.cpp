#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>

#include "../../common/errorcode.h"
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "baiduvideos_response.h"

extern uint64_t g_logid, g_logid_local, g_logid_response, geodb;
extern map<string, uint16_t> dev_make_table;
extern int *numcount;
extern MD5_CTX hash_ctx;


static void get_splash(IN string extinfo, OUT int &adt, OUT string &ade)
{
	json_t *root = NULL;
	json_t *label;
	if (extinfo.size() > 0)
	{
		json_parse_document(&root, extinfo.c_str());

		if (root != NULL)
		{
			if ((label = json_find_first_label(root, "adt")) != NULL && label->child->type == JSON_NUMBER)
				adt = atoi(label->child->text);

			if ((label = json_find_first_label(root, "ade")) != NULL && label->child->type == JSON_STRING)
				if (strlen(label->child->text) != 0)
					ade = label->child->text;

			json_free_value(&root);
		}
	}
}


static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{

	if (crequest.imp.size() > 0)
	{
		double price_gap = (price - crequest.imp[0].bidfloor);

		if (price_gap < 0.01)//1 cent
		{
			return false;
		}
	}

	return true;
}

static bool callback_check_exts(IN const COM_REQUEST &crequest, IN const map<uint8_t, string> &exts)
{
	return true;
}


static void get_video(IN json_t *video_value, OUT COM_VIDEOOBJECT &video)
{
	json_t *label = NULL;

	//获取video对象的mimes(支持播放的视频格式)
	if (video_value->type == JSON_OBJECT)
	{
		if ((label = json_find_first_label(video_value, "mimes")) != NULL && label->child->type == JSON_ARRAY)
		{
			json_t *temp = label->child->child;

			while (temp != NULL && temp->type == JSON_NUMBER)
			{
				video.mimes.insert(atoi(temp->text));
				temp = temp->next;
			}
		}

		//获取video对象的display(广告位类型)
		if ((label = json_find_first_label(video_value, "display")) != NULL && label->child->type == JSON_NUMBER)
		{
			video.display = atoi(label->child->text);
		}

		//获取video对象的minduration(视频广告最小时长)
		if ((label = json_find_first_label(video_value, "minduration")) != NULL && label->child->type == JSON_NUMBER)
		{
			video.minduration = atoi(label->child->text);
		}

		//获取video对象的maxduration(视频广告最大时长)
		if ((label = json_find_first_label(video_value, "maxduration")) != NULL && label->child->type == JSON_NUMBER)
		{
			video.maxduration = atoi(label->child->text);
		}

		//获取video对象的w(广告位宽度)
		if ((label = json_find_first_label(video_value, "w")) != NULL && label->child->type == JSON_NUMBER)
		{
			video.w = atoi(label->child->text);
		}

		//获取video对象的h(广告位高度)
		if ((label = json_find_first_label(video_value, "h")) != NULL && label->child->type == JSON_NUMBER)
		{
			video.h = atoi(label->child->text);
		}

		//获取video对象的battr(拒绝的广告属性)
		if ((label = json_find_first_label(video_value, "battr")) != NULL && label->child->type == JSON_ARRAY)
		{
			json_t *temp = label->child->child;

			while (temp != NULL && temp->type == JSON_NUMBER)
			{
				video.battr.insert(atoi(temp->text));
				temp = temp->next;
			}
		}
	}
}

static void get_native(IN json_t *native_value, OUT COM_NATIVE_REQUEST_OBJECT &native)
{
	json_t *label = NULL;

	if (native_value->type == JSON_OBJECT)
	{
		//初始化natve对象
		init_com_native_request_object(native);

		if ((label = json_find_first_label(native_value, "layout")) != NULL && label->child->type == JSON_NUMBER)
		{
			native.layout = atoi(label->child->text);
		}

		if ((label = json_find_first_label(native_value, "plcmtcnt")) != NULL && label->child->type == JSON_NUMBER)
		{
			native.plcmtcnt = atoi(label->child->text);
		}

		if ((label = json_find_first_label(native_value, "bctype")) != NULL && label->child->type == JSON_ARRAY)
		{
			json_t *temp = label->child->child;

			while (temp != NULL && temp->type == JSON_NUMBER)
			{
				native.bctype.insert(atoi(temp->text));
				temp = temp->next;
			}
		}

		//获取native的assets对象(信息流)
		if ((label = json_find_first_label(native_value, "assets")) != NULL && label->child->type == JSON_ARRAY)
		{
			json_t *temp_asset = label->child->child;

			while (temp_asset != NULL && temp_asset->type == JSON_OBJECT)
			{
				COM_ASSET_REQUEST_OBJECT com_asset;

				//初始化assets对象
				init_com_asset_request_object(com_asset);

				//获取assets的id
				if ((label = json_find_first_label(temp_asset, "id")) != NULL && label->child->type == JSON_NUMBER)
				{
					com_asset.id = atoi(label->child->text);
				}

				//获取assets的require
				if ((label = json_find_first_label(temp_asset, "required")) != NULL && label->child->type == JSON_NUMBER)
				{
					com_asset.required = atoi(label->child->text);
				}

				//获取assets的title(文字元素)
				if ((label = json_find_first_label(temp_asset, "title")) != NULL && label->child->type == JSON_OBJECT)
				{
					json_t *title_value = label->child;

					if (title_value != NULL)
					{
						com_asset.type = NATIVE_ASSETTYPE_TITLE;

						//获取assets的len(Title元素最大文字长度)
						if ((label = json_find_first_label(title_value, "len")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.title.len = atoi(label->child->text);
						}
					}
				}
				//获取assets的img(图片元素)
				else if ((label = json_find_first_label(temp_asset, "img")) != NULL && label->child->type == JSON_OBJECT)
				{
					json_t *img_value = label->child;

					if (img_value != NULL)
					{
						com_asset.type = NATIVE_ASSETTYPE_IMAGE;
						//获取img的type(图片类型)
						if ((label = json_find_first_label(img_value, "type")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.img.type = atoi(label->child->text);
						}

						//获取img的type(图片类型)
						if ((label = json_find_first_label(img_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.img.w = atoi(label->child->text);
						}

						//获取img的wmin(要求的最小宽度)
						if ((label = json_find_first_label(img_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.img.wmin = atoi(label->child->text);
						}

						//获取img的h(高度)
						if ((label = json_find_first_label(img_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.img.h = atoi(label->child->text);
						}

						//获取img的wmin(要求的最小高度)
						if ((label = json_find_first_label(img_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.img.hmin = atoi(label->child->text);
						}

						//获取img的mimes(支持的图片类型)
						if ((label = json_find_first_label(img_value, "mimes")) != NULL && label->child->type == JSON_ARRAY)
						{
							json_t *temp = label->child->child;

							while (temp != NULL && temp->type == JSON_NUMBER)
							{
								com_asset.img.mimes.insert(atoi(temp->text));
								temp = temp->next;
							}
						}
					}


				}
				//获取assets的data对象
				else if ((label = json_find_first_label(temp_asset, "data")) != NULL && label->child->type == JSON_OBJECT)
				{
					json_t *data_value = label->child;

					if (data_value != NULL)
					{
						com_asset.type = NATIVE_ASSETTYPE_DATA;

						//获取data对象的type(数据类型)
						if ((label = json_find_first_label(data_value, "type")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.data.type = atoi(label->child->text);
						}

						//获取data对象的len(元素最大文字长度)
						if ((label = json_find_first_label(data_value, "len")) != NULL && label->child->type == JSON_NUMBER)
						{
							com_asset.data.len = atoi(label->child->text);
						}
					}
				}

				//将信息流广告插入原生广告数组中
				native.assets.push_back(com_asset);
				temp_asset = temp_asset->next;
			}
		}
	}
}

static int parse_pxene_request(RECVDATA *recvdata, COM_REQUEST &request)
{
	json_t *root = NULL;
	json_t *label = NULL;
	int errorcode = E_SUCCESS;

	//将recvdata->data转为JSON格式
	json_parse_document(&root, recvdata->data);
	if (root == NULL)
	{
		errorcode = E_BAD_REQUEST;
		goto exit;
	}

	//获取id
	if ((label = json_find_first_label(root, "id")) != NULL && label->child->type == JSON_STRING)
		request.id = label->child->text;
	else
	{
		errorcode = E_REQUEST_NO_BID;
		goto exit;
	}

	request.is_secure = 0;
	//获取imp对象
	if ((label = json_find_first_label(root, "imp")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *imp_value = label->child->child;    // array

		if (imp_value != NULL && imp_value->type == JSON_OBJECT)
		{
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);

			if ((label = json_find_first_label(imp_value, "id")) != NULL && label->child->type == JSON_STRING)
				imp.id = label->child->text;

			if ((label = json_find_first_label(imp_value, "banner")) != NULL && label->child->type == JSON_OBJECT)
			{
				json_t *banner_value = label->child;

				if (banner_value != NULL)
				{
					imp.type = IMP_BANNER;

					if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.w = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "btype")) != NULL && label->child->type == JSON_ARRAY)
					{
						json_t *temp = label->child->child;

						while (temp != NULL && temp->type == JSON_NUMBER)
						{
							imp.banner.btype.insert(atoi(temp->text));
							temp = temp->next;
						}
					}

					if ((label = json_find_first_label(banner_value, "battr")) != NULL && label->child->type == JSON_ARRAY)
					{
						json_t *temp = label->child->child;
						while (temp != NULL && temp->type == JSON_NUMBER)
						{
							imp.banner.battr.insert(atoi(temp->text));
							temp = temp->next;
						}
					}
				}
			}
			else
			{
				va_cout("other type");
				/* errorcode |= E_INVALID_PARAM;

				 if((label = json_find_first_label(imp_value, "video")) != NULL && label->child->type == JSON_OBJECT)
				 {
				 json_t *video_value = label->child;
				 if(video_value != NULL)
				 {
				 imp.type = IMP_VIDEO;

				 get_video(video_value, imp.video);
				 }
				 }
				 */
				if ((label = json_find_first_label(imp_value, "native")) != NULL && label->child->type == JSON_OBJECT)
				{
					json_t *native_value = label->child;
					if (native_value != NULL)
					{
						imp.type = IMP_NATIVE;

						get_native(native_value, imp.native);
					}
				}
			}


			if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
			{
				imp.bidfloor = atoi(label->child->text) / 100.0;
			}


			if ((label = json_find_first_label(imp_value, "bidfloorcur")) != NULL && label->child->type == JSON_STRING)
				imp.bidfloorcur = label->child->text;

			if ((label = json_find_first_label(imp_value, "ext")) != NULL && label->child->type == JSON_OBJECT)
			{
				json_t *ext_value = label->child;

				if (ext_value != NULL)
				{
					if ((label = json_find_first_label(ext_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
						imp.ext.instl = atoi(label->child->text);

					if ((label = json_find_first_label(ext_value, "splash")) != NULL && label->child->type == JSON_NUMBER)
						imp.ext.splash = atoi(label->child->text);
				}
			}

			request.imp.push_back(imp);
		}
	}

	//获取app对象
	if ((label = json_find_first_label(root, "app")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *app_child = label->child;

		if (app_child != NULL)
		{

			if ((label = json_find_first_label(app_child, "id")) != NULL && label->child->type == JSON_STRING)
				request.app.id = label->child->text;


			if ((label = json_find_first_label(app_child, "name")) != NULL && label->child->type == JSON_STRING)
				request.app.name = label->child->text;


			if ((label = json_find_first_label(app_child, "cat")) != NULL && label->child->type == JSON_ARRAY)
			{
				json_t *temp = label->child->child;

				while (temp != NULL && temp->type == JSON_NUMBER)
				{
					request.app.cat.insert(atoi(temp->text));
					temp = temp->next;
				}
			}


			if ((label = json_find_first_label(app_child, "bundle")) != NULL && label->child->type == JSON_STRING)
				request.app.bundle = label->child->text;

			if ((label = json_find_first_label(app_child, "storeurl")) != NULL && label->child->type == JSON_STRING)
				request.app.storeurl = label->child->text;
		}
	}

	//获取device对象(设备)
	if ((label = json_find_first_label(root, "device")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *device_child = label->child;

		if (device_child != NULL)
		{

			if ((label = json_find_first_label(device_child, "ua")) != NULL && label->child->type == JSON_STRING)
			{
				request.device.ua = label->child->text;
			}


			if ((label = json_find_first_label(device_child, "ip")) != NULL && label->child->type == JSON_STRING)
			{
				request.device.ip = label->child->text;
				if (request.device.ip != "")
				{
					int location = getRegionCode(geodb, request.device.ip);
					request.device.location = location;
				}
			}


			if ((label = json_find_first_label(device_child, "geo")) != NULL && label->child->type == JSON_OBJECT)
			{
				json_t *geo_value = label->child;

				if (geo_value != NULL)
				{

					if ((label = json_find_first_label(geo_value, "lat")) != NULL && label->child->type == JSON_NUMBER)
						request.device.geo.lat = atof(label->child->text);// atof to float???

					if ((label = json_find_first_label(geo_value, "lon")) != NULL && label->child->type == JSON_NUMBER)
						request.device.geo.lon = atof(label->child->text);
				}
			}


			if ((label = json_find_first_label(device_child, "dpids")) != NULL && label->child->type == JSON_ARRAY)
			{
				json_t *dpids = label->child->child;

				while (dpids != NULL && dpids->type == JSON_OBJECT)
				{
					int type = DPID_UNKNOWN;
					string id;
					if ((label = json_find_first_label(dpids, "type")) != NULL && label->child->type == JSON_NUMBER)
						type = atoi(label->child->text);

					if ((label = json_find_first_label(dpids, "id")) != NULL && label->child->type == JSON_STRING)
						id = label->child->text;
					if (id != "")
						request.device.dpids.insert(make_pair(type, id));
					dpids = dpids->next;
				}
			}


			if ((label = json_find_first_label(device_child, "dids")) != NULL && label->child->type == JSON_ARRAY)
			{
				json_t *dids = label->child->child;

				while (dids != NULL && dids->type == JSON_OBJECT)
				{
					int type = DID_UNKNOWN;
					string id;
					if ((label = json_find_first_label(dids, "type")) != NULL && label->child->type == JSON_NUMBER)
						type = atoi(label->child->text);

					if ((label = json_find_first_label(dids, "id")) != NULL && label->child->type == JSON_STRING)
						id = label->child->text;
					if (id != "")
						request.device.dids.insert(make_pair(type, id));
					dids = dids->next;
				}
			}


			if ((label = json_find_first_label(device_child, "carrier")) != NULL && label->child->type == JSON_NUMBER)
				request.device.carrier = atoi(label->child->text);


			if ((label = json_find_first_label(device_child, "make")) != NULL && label->child->type == JSON_STRING)
			{
				string s_make = label->child->text;
				if (s_make != "")
				{
					map<string, uint16_t>::iterator it;

					request.device.makestr = s_make;
					for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
					{
						if (s_make.find(it->first) != string::npos)
						{
							request.device.make = it->second;
							break;
						}
					}
				}
			}


			if ((label = json_find_first_label(device_child, "model")) != NULL && label->child->type == JSON_STRING)
				request.device.model = label->child->text;


			if ((label = json_find_first_label(device_child, "os")) != NULL && label->child->type == JSON_NUMBER)
				request.device.os = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "osv")) != NULL && label->child->type == JSON_STRING)
				request.device.osv = label->child->text;


			if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
				request.device.connectiontype = atoi(label->child->text);


			if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
				request.device.devicetype = atoi(label->child->text);
		}
	}

	request.cur.insert(string("CNY"));


	if ((label = json_find_first_label(root, "bcat")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *temp = label->child->child;

		while (temp != NULL && temp->type == JSON_NUMBER)
		{
			request.bcat.insert(atoi(temp->text));
			temp = temp->next;
		}
	}


	if ((label = json_find_first_label(root, "badv")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *temp = label->child->child;

		while (temp != NULL && temp->type == JSON_STRING)
		{
			request.badv.insert(temp->text);
			temp = temp->next;
		}
	}

exit:
	if (root != NULL)
		json_free_value(&root);

	return errorcode;
}

static bool setJsonResponse(COM_REQUEST &request, COM_RESPONSE &response, ADXTEMPLATE &adxtemplate, string &response_out)
{
	char *text = NULL;
	json_t *root = NULL, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;
	string trace_url_par = string("&") + get_trace_url_par(request, response);
	//新建JSON对象
	root = json_new_object();

	if (root == NULL)
		goto exit;

	//插入response的id
	jsonInsertString(root, "id", response.id.c_str());

	if (!response.seatbid.empty())
	{

		//DSP的seatbid对象
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < response.seatbid.size(); ++i)
		{
			char *seatbid_text;
			//每次取出DSP的seatbid对象
			COM_SEATBIDOBJECT *seatbid = &response.seatbid[i];
			entry_seatbid = json_new_object();

			//DSP的出价不为空
			if (seatbid->bid.size() > 0)
			{
				//获取DSP出价对象
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid->bid.size(); ++j)
				{
					char *bid_text;
					//获取DSP出价
					COM_BIDOBJECT *bid = &seatbid->bid[j];
					entry_bid = json_new_object();

					//获取mapid(创意id)
					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					//获取price(出价)
					jsonInsertInt(entry_bid, "price", bid->price * 100);//%.6lf
					//获取广告id(后台用广告id)
					jsonInsertString(entry_bid, "adid", bid->adid.c_str());
					//获取广告w和h
					jsonInsertInt(entry_bid, "w", bid->w);
					jsonInsertInt(entry_bid, "h", bid->h);
					//获取cid(Campaign ID)
					jsonInsertString(entry_bid, "cid", bid->cid.c_str());
					//获取crid(广告物料ID)
					jsonInsertString(entry_bid, "crid", bid->crid.c_str());
					//获取ctype(点击目标类型)
					jsonInsertInt(entry_bid, "ctype", bid->ctype);
					//获取adomain(广告主检查主域名或顶级域名)
					jsonInsertString(entry_bid, "adomain", bid->adomain.c_str());
					//
					jsonInsertString(entry_bid, "admark", "广告");
					//
					jsonInsertString(entry_bid, "adsource", "蓬景");

					//遍历cat数组(广告行业类型)
					if (bid->cat.size() > 0)
					{
						json_t *label_cat, *array_cat, *value_cat;

						label_cat = json_new_string("cat");
						array_cat = json_new_array();
						set<uint32_t>::iterator it;
						for (it = bid->cat.begin(); it != bid->cat.end(); ++it)
						{
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							value_cat = json_new_number(buf);
							json_insert_child(array_cat, value_cat);
						}
						json_insert_child(label_cat, array_cat);
						json_insert_child(entry_bid, label_cat);

					}

					//遍历attr数组(广告属性)
					if (bid->attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						set<uint8_t>::iterator it;
						for (it = bid->attr.begin(); it != bid->attr.end(); ++it)
						{
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							value_attr = json_new_number(buf);
							json_insert_child(array_attr, value_attr);
						}
						json_insert_child(label_attr, array_attr);
						json_insert_child(entry_bid, label_attr);
					}

					//遍历iurl数组(DSP及第三方广告展示监测地址，在第一个展示监测地址中存在#WIN_PRICE#宏，用于通知DSP成交价)
					if (adxtemplate.iurl.size() > 0 || bid->imonitorurl.size() > 0)
					{
						//ADX模板中替换宏
						json_t *label_iurl, *array_iurl, *value_iurl;
						label_iurl = json_new_string("iurl");
						array_iurl = json_new_array();

                        for (uint32_t k = 0; k < adxtemplate.iurl.size(); ++k)
                        {
                            string iurl = adxtemplate.iurl[k] + trace_url_par;
                            value_iurl = json_new_string(iurl.c_str());
                            json_insert_child(array_iurl, value_iurl);
                        }

						//iURL中替换第三方的宏定义
						for (uint32_t k = 0; k < bid->imonitorurl.size(); ++k)
						{
							value_iurl = json_new_string(bid->imonitorurl[k].c_str());
							json_insert_child(array_iurl, value_iurl);
						}

						json_insert_child(label_iurl, array_iurl);
						json_insert_child(entry_bid, label_iurl);
					}

					//获取aurl(激活效果地址，用于 CPA 广告的激活效果回调)
					//if (adxtemplate.aurl != "")
					//{
					//   string aurl;
					//    aurl = adxtemplate.aurl;
					//    replace_macro(aurl, request.id, request.device.deviceid, bid->mapid, request.device.deviceidtype);
					//    jsonInsertString(entry_bid, "aurl", aurl.c_str());
					//}

					//获取curl(广告点击落地页)
					uint32_t direct_i = 0;
					string curl = bid->curl;

                    if (bid->redirect)
                    {
                        if (adxtemplate.cturl.size() > 0)
                        {
                            string cturl = adxtemplate.cturl[0] + trace_url_par;
                            ++direct_i;
                            string strcurl = curl;
                            int len = 0;
                            char *curl_r = urlencode(strcurl.c_str(),strcurl.size(),&len);
                            cturl = cturl + "&curl=" + string(curl_r,len);
                            free(curl_r);
                            curl = cturl;
                        }
                    }
                    jsonInsertString(entry_bid, "curl", curl.c_str());

					//获取cturl(DSP及第三方广告点击监测地址)
                    if(1)
                    {
                        json_t *label_cturl, *array_cturl, *value_cturl;
                        label_cturl = json_new_string("cturl");
                        array_cturl = json_new_array();
                        for (uint32_t k = direct_i; k < adxtemplate.cturl.size(); ++k)
                        {
                            string cturl = adxtemplate.cturl[k] + trace_url_par;
                            value_cturl = json_new_string(cturl.c_str());
                            json_insert_child(array_cturl, value_cturl);
                        }

						for (uint32_t k = 0; k < bid->cmonitorurl.size(); ++k)
						{
							value_cturl = json_new_string(bid->cmonitorurl[k].c_str());
							json_insert_child(array_cturl, value_cturl);
						}
						json_insert_child(label_cturl, array_cturl);
						json_insert_child(entry_bid, label_cturl);
					}

					//获取banner对象(广告物料，Banner广告必填)
					if (request.imp[0].type == IMP_BANNER)
					{
						json_t *label_banner, *entry_banner;
						label_banner = json_new_string("banner");
						entry_banner = json_new_object();
						jsonInsertString(entry_banner, "url", bid->sourceurl.c_str());
						json_insert_child(label_banner, entry_banner);
						json_insert_child(entry_bid, label_banner);
					}
					//获取video对象(广告物料，Video广告必填)
					else if (request.imp[0].type == IMP_VIDEO)
					{
						json_t *label_video, *entry_video;
						label_video = json_new_string("video");
						entry_video = json_new_object();
						jsonInsertString(entry_video, "url", bid->sourceurl.c_str());
						json_insert_child(label_video, entry_video);
						json_insert_child(entry_bid, label_video);
					}
					//获取native对象(广告物料，Native广告必填)
					else if (request.imp[0].type == IMP_NATIVE)
					{
						json_t *label_native, *array_native, *entry_native, *label_assets, *value_native;
						label_native = json_new_string("native");
						value_native = json_new_object();

						//获取native的assets对象
						label_assets = json_new_string("assets");
						array_native = json_new_array();

						for (uint32_t k = 0; k < bid->native.assets.size(); ++k)
						{
							entry_native = json_new_object();
							COM_ASSET_RESPONSE_OBJECT *asset = &bid->native.assets[k];

							//获取assets的id(广告元素ID)
							jsonInsertInt(entry_native, "id", asset->id);

							if (asset->type == NATIVE_ASSETTYPE_TITLE)
							{
								json_t *label_title, *value_title;
								//获取assets的title对象(文字元素，title元素必填)
								label_title = json_new_string("title");
								value_title = json_new_object();
								//获取titile的text(Title元素的内容文字)
								jsonInsertString(value_title, "text", asset->title.text.c_str());
								json_insert_child(label_title, value_title);
								json_insert_child(entry_native, label_title);
							}
							else if (asset->type == NATIVE_ASSETTYPE_IMAGE)
							{
								json_t *label_img, *value_img;
								//获取assets的img对象(图片元素，img元素必填。)
								label_img = json_new_string("img");
								value_img = json_new_object();
								//获取img的url(Image元素的URL地址)
								jsonInsertString(value_img, "url", asset->img.url.c_str());
								//获取img的w和h(Image元素的宽度和高度)
								jsonInsertInt(value_img, "w", asset->img.w);
								jsonInsertInt(value_img, "h", asset->img.h);
								json_insert_child(label_img, value_img);
								json_insert_child(entry_native, label_img);
							}
							else if (asset->type == NATIVE_ASSETTYPE_DATA)
							{
								json_t *label_data, *value_data;
								//获取assets的data对象(其他数据元素，data元素必填)
								label_data = json_new_string("data");
								value_data = json_new_object();
								//获取data的label(数据显示的名称)
								jsonInsertString(value_data, "label", asset->data.label.c_str());
								//获取data的value(数据的内容文字)
								jsonInsertString(value_data, "value", asset->data.value.c_str());
								json_insert_child(label_data, value_data);
								json_insert_child(entry_native, label_data);
							}
							json_insert_child(array_native, entry_native);
						}
						json_insert_child(label_assets, array_native);
						json_insert_child(value_native, label_assets);
						json_insert_child(label_native, value_native);
						json_insert_child(entry_bid, label_native);
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

	json_tree_to_string(root, &text);

	response_out = text;

exit:
	if (text != NULL)
		free(text);

	if (root != NULL)
		json_free_value(&root);

}



int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata,
	OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int errcode = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST crequest;
	COM_RESPONSE cresponse;
	ADXTEMPLATE adxtemp;
	string output;
	struct timespec ts1, ts2;
	long lasttime;
	string str_ferrcode = "";

	//入参检查
	if ((recvdata == NULL) || (ctx == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata or ctx is null");
		errcode = E_BID_PROGRAM;
		goto release;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata->data is null or recvdata->length is 0");
		errcode = E_BID_PROGRAM;
		goto release;
	}

	getTime(&ts1);
	//初始化COMMON_REQUEST
	init_com_message_request(&crequest);

	//将ADX_REQUEST转为COMMON_REQUEST
	errcode = parse_pxene_request(recvdata, crequest);
	if (errcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "Parse req data failed!");
		goto exit;
	}
	flow_bid.set_req(crequest);

	//无法进行区域定向
	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		errcode = E_REQUEST_DEVICE_IP;
		goto exit;
	}

	init_com_message_response(&cresponse);
	init_com_template_object(&adxtemp);

	ferrcode = bid_filter_response(ctx, index, crequest, NULL, NULL, callback_check_price, NULL, &cresponse, &adxtemp);
	errcode = ferrcode.errcode;
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	flow_bid.set_bid_flow(ferrcode);
	g_ser_log.send_bid_log(index, crequest, cresponse, errcode);

exit:
	if (errcode == E_SUCCESS)
	{
		setJsonResponse(crequest, cresponse, adxtemp, output);

		senddata = "Content-Length: " + intToString(output.size()) + "\r\n\r\n" + output;
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf",
			cresponse.bidid.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(), 
			crequest.device.deviceid.c_str(), crequest.device.deviceidtype, 
			cresponse.seatbid[0].bid[0].price);
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));

		numcount[index]++;
		if (numcount[index] % 1000 == 0)
		{
			writeLog(g_logid_local, LOGDEBUG, "response :" + output);
			numcount[index] = 0;
		}
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s, spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());

release:
	flow_bid.set_err(errcode);
	return errcode;
}
