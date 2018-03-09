/*
 * inmobi_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>

#include "../../common/errorcode.h"
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "iflytek_response.h"

extern uint64_t g_logid_local, g_logid_local_traffic, g_logid, g_logid_response;
extern map<string, vector<uint32_t> > app_cat_table_adx2com;
extern map<string, vector<uint32_t> > adv_cat_table_adx2com;
//extern map<uint32_t, vector<string> > adv_cat_table_com2adx;
extern map<string, uint16_t> dev_make_table;
extern MD5_CTX hash_ctx;
extern uint64_t geodb;

static void init_geo_object(GEOOBJECT &geo)//
{
	geo.lat = geo.lon = 0;
}

static void init_user_object(USEROBJECT &user)//
{
	user.yob = -1;
}

static void init_app_object(APPOBJECT &app)//
{
}

static void init_device_ext(DEVICEEXT &ext)//
{
	ext.sw = ext.sh = ext.brk = 0;
}
static void init_device_object(DEVICEOBJECT &device)//
{
	device.js = device.connectiontype = device.devicetype = 0;
	init_device_ext(device.ext);
	init_geo_object(device.geo);
}

static void init_title_object(TITLEOBJECT &title)
{
	title.len = 0;
}

static void init_desc_object(DESCOBJECT &desc)
{
	desc.len = 0;
}

static void init_icon_object(ICONOBJECT &icon)
{
	icon.w = icon.h = icon.wmin = icon.hmin = 0;
}

static void init_img_object(IMGOBJECT &img)
{
	img.w = img.h = img.wmin = img.hmin = 0;
}

static void init_native_request(NATIVEREQUEST &request)
{
	init_title_object(request.title);

	init_desc_object(request.desc);

	init_icon_object(request.icon);

	init_img_object(request.img);
}
static void init_native_ext(NATIVEEXT &ext)
{
}
static void init_native_object(NATIVEOBJECT &native)
{
	native.battr = 0;

	init_native_request(native.natreq);

	init_native_ext(native.ext);
}

static void init_video_ext(VIDEOEXT &ext)
{
}
static void init_video_object(VIDEOOBJECT &video)
{
	video.protocol = video.w = video.h = video.minduration = video.maxduration = video.linearity = 0;

	init_video_ext(video.ext);
}

static void init_banner_ext(BANNEREXT &ext)
{
}
static void init_banner_object(BANNEROBJECT &banner)
{
	banner.w = banner.h = 0;

	init_banner_ext(banner.ext);
}

static void init_impression_object(IMPRESSIONOBJECT &imp)
{
	imp.instl = imp.bidfloor = 0;

	init_banner_object(imp.banner);

	init_video_object(imp.video);

	init_native_object(imp.native);
}

void init_message_request(MESSAGEREQUEST &mrequest)//
{

	init_user_object(mrequest.user);

	init_app_object(mrequest.app);

	init_device_object(mrequest.device);

	mrequest.at = 1;

	mrequest.tmax = 0;

	mrequest.is_secure = 0;
}

static void init_seat_bid(SEATBIDOBJECT &seatbid)
{
}

static void init_bid_object(BIDOBJECT &bid)
{
	bid.lattr = bid.w = bid.h = 0;
	bid.video.duration = 0;
}

static void init_message_response(MESSAGERESPONSE &response)
{
}

static void revert_json(string &text)
{
	replaceMacro(text, "\\", "");
}

static void serial_json(string macro, string data, string &text)
{
	int locate = 0;
	while (1)
	{

		locate = text.find(macro.c_str(), locate);
		if (locate != string::npos)
		{
			text.replace(locate, macro.size(), data.c_str());
			locate += data.size();
		}
		else
			break;
	}
	return;
}

bool parse_iflytek_request(char *recvdata, MESSAGEREQUEST &request, int &imptype)
{
	json_t *root = NULL;
	json_t *label = NULL;

	json_parse_document(&root, recvdata);
	if (root == NULL)
		return false;

	imptype = IMP_UNKNOWN;

	if ((label = json_find_first_label(root, "id")) != NULL)
		request.id = label->child->text;

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;   // array,get first

		if (imp_value != NULL)
		{
			IMPRESSIONOBJECT imp;

			init_impression_object(imp);

			if ((label = json_find_first_label(imp_value, "id")) != NULL)
				imp.id = label->child->text;

			if ((label = json_find_first_label(imp_value, "secure")) != NULL &&
				label->child->type == JSON_NUMBER)
				request.is_secure = atoi(label->child->text);

			if ((label = json_find_first_label(imp_value, "banner")) != NULL)
			{
				json_t *banner_value = label->child;

				if (banner_value != NULL)
				{
					//BANNEROBJECT bannerobject;

					imptype = IMP_BANNER;
					if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.w = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "ext")) != NULL)
					{
					}
				}
			}
			// video
			if ((label = json_find_first_label(imp_value, "video")) != NULL)
			{
				json_t *video_value = label->child;

				if (video_value != NULL)
				{
					//VIDEOROBJECT bannerobject;
					imptype = IMP_VIDEO;
					if ((label = json_find_first_label(video_value, "protocol")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.protocol = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.w = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.h = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "minduration")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.minduration = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "maxduration")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.maxduration = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "linearity")) != NULL && label->child->type == JSON_NUMBER)
						imp.video.linearity = atoi(label->child->text);

					if ((label = json_find_first_label(video_value, "ext")) != NULL)
					{
					}
				}
			}

			if ((label = json_find_first_label(imp_value, "native")) != NULL)
			{
				json_t *native_value = label->child;

				if (native_value != NULL)
				{
					//VIDEOROBJECT bannerobject;
					imptype = IMP_NATIVE;

					if ((label = json_find_first_label(imp_value, "tagid")) != NULL)
						writeLog(g_logid_local, LOGDEBUG, "id=%s,pid=%s", request.id.c_str(), label->child->text);

					if ((label = json_find_first_label(native_value, "request")) != NULL)
					{
						imp.native.request = label->child->text;

						json_t *request_root = NULL;
						string label_text = label->child->text;
						revert_json(label_text);
						json_parse_document(&request_root, label_text.c_str());
						if (request_root != NULL)
						{
							if ((label = json_find_first_label(request_root, "title")) != NULL)
							{
								json_t *title_value = label->child;
								if (title_value != NULL)
								{
									imp.native.natreq.assettype.push_back(1);
									if ((label = json_find_first_label(title_value, "len")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.title.len = atoi(label->child->text);
								}
							}

							if ((label = json_find_first_label(request_root, "desc")) != NULL)
							{
								json_t *desc_value = label->child;
								if (desc_value != NULL)
								{
									imp.native.natreq.assettype.push_back(2);
									if ((label = json_find_first_label(desc_value, "len")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.desc.len = atoi(label->child->text);
								}
							}

							if ((label = json_find_first_label(request_root, "icon")) != NULL)
							{
								json_t *icon_value = label->child;
								if (icon_value != NULL)
								{
									imp.native.natreq.assettype.push_back(3);
									if ((label = json_find_first_label(icon_value, "w")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.icon.w = atoi(label->child->text);

									if ((label = json_find_first_label(icon_value, "h")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.icon.h = atoi(label->child->text);

									if ((label = json_find_first_label(icon_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.icon.wmin = atoi(label->child->text);

									if ((label = json_find_first_label(icon_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.icon.hmin = atoi(label->child->text);

									if ((label = json_find_first_label(icon_value, "mimes")) != NULL)
									{
										json_t *tmp = label->child->child;
										while (tmp != NULL)
										{
											imp.native.natreq.icon.mimes.push_back((tmp->text));
											tmp = tmp->next;
										}
									}
								}
							}

							if ((label = json_find_first_label(request_root, "img")) != NULL)
							{
								json_t *img_value = label->child;
								if (img_value != NULL)
								{
									imp.native.natreq.assettype.push_back(4);
									if ((label = json_find_first_label(img_value, "w")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.img.w = atoi(label->child->text);

									if ((label = json_find_first_label(img_value, "h")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.img.h = atoi(label->child->text);

									if ((label = json_find_first_label(img_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.img.wmin = atoi(label->child->text);

									if ((label = json_find_first_label(img_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
										imp.native.natreq.img.hmin = atoi(label->child->text);

									if ((label = json_find_first_label(img_value, "mimes")) != NULL)
									{
										json_t *tmp = label->child->child;
										while (tmp != NULL)
										{
											imp.native.natreq.img.mimes.push_back((tmp->text));
											tmp = tmp->next;
										}
									}
								}
							}

							json_free_value(&request_root);
						}
					}

					if ((label = json_find_first_label(native_value, "ver")) != NULL)
						imp.native.ver = (label->child->text);

					if ((label = json_find_first_label(native_value, "api")) != NULL)
					{
						json_t *temp = label->child->child;

						while (temp != NULL)
						{
							imp.native.api.push_back(atoi(temp->text));
							temp = temp->next;
						}
					}

					if ((label = json_find_first_label(native_value, "battr")) != NULL && label->child->type == JSON_NUMBER)
						imp.native.battr = atoi(label->child->text);

					if ((label = json_find_first_label(native_value, "ext")) != NULL)
					{
					}
				}
			}

			if ((label = json_find_first_label(imp_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
				imp.instl = atoi(label->child->text);

			if ((label = json_find_first_label(imp_value, "lattr")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					imp.lattr.push_back(atoi(temp->text));
					temp = temp->next;
				}
			}

			if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
			{
				imp.bidfloor = atof(label->child->text);
			}

			if ((label = json_find_first_label(imp_value, "ext")) != NULL)
			{
			}

			request.imp.push_back(imp);
		}
	}


	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if ((label = json_find_first_label(app_child, "id")) != NULL)
			request.app.id = label->child->text;

		if ((label = json_find_first_label(app_child, "name")) != NULL)
			request.app.name = label->child->text;

		if ((label = json_find_first_label(app_child, "cat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.app.cat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(app_child, "ver")) != NULL)
			request.app.ver = label->child->text;

		if ((label = json_find_first_label(app_child, "bundle")) != NULL)
			request.app.bundle = label->child->text;

		if ((label = json_find_first_label(app_child, "paid")) != NULL)
			request.app.paid = (label->child->text);
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;

		if (device_child != NULL)
		{
			if ((label = json_find_first_label(device_child, "ua")) != NULL)
				request.device.ua = label->child->text;

			if ((label = json_find_first_label(device_child, "ip")) != NULL)
				request.device.ip = label->child->text;

			if ((label = json_find_first_label(device_child, "geo")) != NULL)
			{
				json_t *geo_value = label->child;

				if ((label = json_find_first_label(geo_value, "lat")) != NULL && label->child->type == JSON_NUMBER)
					request.device.geo.lat = atof(label->child->text);// atof to float???

				if ((label = json_find_first_label(geo_value, "lon")) != NULL && label->child->type == JSON_NUMBER)
					request.device.geo.lon = atof(label->child->text);

				if ((label = json_find_first_label(geo_value, "country")) != NULL)
					request.device.geo.country = label->child->text;

				if ((label = json_find_first_label(geo_value, "city")) != NULL)
					request.device.geo.city = label->child->text;
			}

			if ((label = json_find_first_label(device_child, "didsha1")) != NULL)
				request.device.didsha1 = label->child->text;

			if ((label = json_find_first_label(device_child, "didmd5")) != NULL)
				request.device.didmd5 = label->child->text;

			if ((label = json_find_first_label(device_child, "dpidsha1")) != NULL)
				request.device.dpidsha1 = label->child->text;

			if ((label = json_find_first_label(device_child, "dpidmd5")) != NULL)
				request.device.dpidmd5 = label->child->text;

			if ((label = json_find_first_label(device_child, "mac")) != NULL)
				request.device.mac = label->child->text;

			if ((label = json_find_first_label(device_child, "macsha1")) != NULL)
				request.device.macsha1 = label->child->text;

			if ((label = json_find_first_label(device_child, "macmd5")) != NULL)
				request.device.macmd5 = label->child->text;

			if ((label = json_find_first_label(device_child, "ifa")) != NULL)
				request.device.ifa = label->child->text;


			if ((label = json_find_first_label(device_child, "carrier")) != NULL)
				request.device.carrier = label->child->text;

			if ((label = json_find_first_label(device_child, "language")) != NULL)
				request.device.language = label->child->text;

			if ((label = json_find_first_label(device_child, "make")) != NULL)
				request.device.make = label->child->text;

			if ((label = json_find_first_label(device_child, "model")) != NULL)
				request.device.model = label->child->text;

			if ((label = json_find_first_label(device_child, "os")) != NULL)
				request.device.os = label->child->text;

			if ((label = json_find_first_label(device_child, "osv")) != NULL)
				request.device.osv = label->child->text;

			if ((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
				request.device.js = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
				request.device.connectiontype = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
				request.device.devicetype = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "ext")) != NULL)
			{
				json_t *ext_value = label->child;

				if ((label = json_find_first_label(ext_value, "sw")) != NULL && label->child->type == JSON_NUMBER)
					request.device.ext.sw = atoi(label->child->text);

				if ((label = json_find_first_label(ext_value, "sh")) != NULL && label->child->type == JSON_NUMBER)
					request.device.ext.sh = atoi(label->child->text);

				if ((label = json_find_first_label(ext_value, "brk")) != NULL && label->child->type == JSON_NUMBER)
					request.device.ext.brk = atoi(label->child->text);
			}
		}
	}

	if ((label = json_find_first_label(root, "user")) != NULL)
	{
		json_t *user_child = label->child;

		if ((label = json_find_first_label(user_child, "id")) != NULL)
			request.user.id = label->child->text;

		if ((label = json_find_first_label(user_child, "gender")) != NULL)
			request.user.gender = label->child->text;

		if ((label = json_find_first_label(user_child, "yob")) != NULL && label->child->type == JSON_NUMBER)
			request.user.yob = atoi(label->child->text);

		if ((label = json_find_first_label(user_child, "keywords")) != NULL && label->child->type == JSON_ARRAY)
		{
			json_t *temp = label->child->child;
			string keywords;
			while (temp != NULL && temp->type == JSON_STRING)
			{
				keywords += temp->text;
				keywords += ",";
				temp = temp->next;
			}
			size_t str_index = keywords.rfind(",");
			if (str_index != string::npos)
			{
				keywords.erase(str_index, 1);
			}

			request.user.keywords = keywords;
		}
	}

	if ((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
		request.at = atoi(label->child->text);

	if ((label = json_find_first_label(root, "tmax")) != NULL && label->child->type == JSON_NUMBER)
		request.tmax = atoi(label->child->text);

	if ((label = json_find_first_label(root, "wseat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.wseat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "cur")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.cur.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "bcat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.bcat.push_back(temp->text);
			temp = temp->next;
		}
	}

	json_free_value(&root);

	return true;
}

static bool setJsonResponse(MESSAGERESPONSE &response, string &response_out)
{
	char *text = NULL;
	json_t *root, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;

	root = json_new_object();
	if (root == NULL)
		return false;
	jsonInsertString(root, "id", response.id.c_str());
	if (!response.seatbid.empty())
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < response.seatbid.size(); i++)
		{
			char *seatbid_text;
			SEATBIDOBJECT *seatbid = &response.seatbid[i];
			entry_seatbid = json_new_object();
			if (seatbid->bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid->bid.size(); j++)
				{
					char *bid_text;
					BIDOBJECT *bid = &seatbid->bid[j];
					entry_bid = json_new_object();

					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					jsonInsertFloat(entry_bid, "price", bid->price, 6);//%.6lf
					//		jsonInsertString(entry_bid, "nurl", bid->nurl.c_str());
					jsonInsertString(entry_bid, "adm", bid->adm.c_str());

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

	jsonInsertString(root, "bidid", response.bidid.c_str());

	jsonInsertString(root, "cur", response.cur.c_str());

	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		response_out = text;
		free(text);
		text = NULL;
	}

	json_free_value(&root);
	return true;
}
/*
uint64_t iab_to_uint64(string cat)
{
int icat = 0, isubcat = 0;
sscanf(cat.c_str(), "IAB%x-%x", &icat, &isubcat);
return (icat << 8) | isubcat;
}

void callback_appcat_insert_pair_range(IN void *data, const string key_start, const string key_end, const string val)
{
map<uint32_t, vector<uint32_t> > &table = *((map<uint32_t, vector<uint32_t> > *)data);

uint32_t com_cat;
sscanf(val.c_str(), "0x%x", &com_cat);//0x%x

uint64_t u64_start = iab_to_uint64(key_start);
uint64_t u64_end = iab_to_uint64(key_end);

assert(u64_start <= u64_end);

for(uint64_t i = u64_start; i <= u64_end; ++i)
{
vector<uint32_t> &s_val = table[i];

s_val.push_back(com_cat);

va_cout("%s: insert: 0x%x -> 0x%x", __FUNCTION__, i, com_cat);
}
}
*/

void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	//	va_cout("%s, key_start:%s, val:%s, com_cat:0x%x", __FUNCTION__, key_start.c_str(), val.c_str(), com_cat);

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

void callback_insert_pair_hex_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<string> > &table = *((map<uint32_t, vector<string> > *)data);

	uint32_t com_cat;
	sscanf(key_start.c_str(), "%x", &com_cat);//0x%x

	//	va_cout("%s, key_start:%s, val:%s, com_cat:0x%x", __FUNCTION__, key_start.c_str(), val.c_str(), com_cat);

	vector<string> &s_val = table[com_cat];

	s_val.push_back(val);
}

string transform_advcat_com_to_adx(uint16_t cat)
{
	return "";
}

uint8_t transform_carrier(string carrier)
{
	if (carrier == "46000" || carrier == "46020")
		return CARRIER_CHINAMOBILE;

	if (carrier == "46001")
		return CARRIER_CHINAUNICOM;

	if (carrier == "46003")
		return CARRIER_CHINATELECOM;

	return CARRIER_UNKNOWN;
}

uint8_t transform_connectiontype(uint8_t connectiontype)
{
	uint8_t com_connectiontype = CONNECTION_UNKNOWN;

	switch (connectiontype)
	{
	case 2:
		com_connectiontype = CONNECTION_WIFI;
		break;
	case 3:
		com_connectiontype = CONNECTION_CELLULAR_2G;
		break;
	case 4:
		com_connectiontype = CONNECTION_CELLULAR_3G;
		break;
	case 5:
		com_connectiontype = CONNECTION_CELLULAR_4G;
		break;
	default:
		com_connectiontype = CONNECTION_UNKNOWN;
		break;
	}

	return com_connectiontype;
}

uint8_t transform_devicetype(uint8_t devicetype)
{
	uint8_t com_devicetype = DEVICE_UNKNOWN;

	switch (devicetype)
	{
	case 0://Phone v2.2
		com_devicetype = DEVICE_PHONE;
		break;
	case 1://Tablet v2.2
		com_devicetype = DEVICE_TABLET;
		break;
	default:
		com_devicetype = DEVICE_UNKNOWN;
		break;
	}

	return com_devicetype;
}

void transform_native_mimes(IN vector<string> &inmimes, OUT set<uint8_t> &outmimes)
{
	for (int i = 0; i < inmimes.size(); ++i)
	{
		string temp = inmimes[i];
		if (temp.find("jpg") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_JPG);
		else if (temp.find("jpeg") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_JPG);
		else if (temp.find("png") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_PNG);
		else if (temp.find("gif") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_GIF);
	}
}

void transform_request(IN MESSAGEREQUEST &m, IN int imptype, OUT COM_REQUEST &c)
{
	int i, j;

	//id
	c.id = m.id;
	c.is_secure = m.is_secure;
	//imp
	for (i = 0; i < m.imp.size(); i++)
	{
		IMPRESSIONOBJECT &imp = m.imp[i];
		COM_IMPRESSIONOBJECT cimp;

		init_com_impression_object(&cimp);

		cimp.id = imp.id;
		cimp.type = imptype;

		if (imptype == IMP_BANNER)
		{
			cimp.requestidlife = 3600;
			cimp.banner.w = imp.banner.w;
			cimp.banner.h = imp.banner.h;
		}
		else if (imptype == IMP_VIDEO)
		{
			cimp.requestidlife = 7200;
			cimp.video.w = imp.video.w;
			cimp.video.h = imp.video.h;
			cimp.video.maxduration = imp.video.maxduration;
			cimp.video.minduration = imp.video.minduration;
		}
		else if (imptype == IMP_NATIVE)
		{
			cimp.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
			cimp.native.plcmtcnt = 1;
			cimp.requestidlife = 7200;
			for (int k = 0; k < imp.native.natreq.assettype.size(); ++k)
			{
				switch (imp.native.natreq.assettype[k])
				{
				case 1:
				{
					COM_ASSET_REQUEST_OBJECT asset_title;
					init_com_asset_request_object(asset_title);
					asset_title.id = 1;
					asset_title.required = 1;
					asset_title.type = NATIVE_ASSETTYPE_TITLE;
					asset_title.title.len = imp.native.natreq.title.len * 3;
					cimp.native.assets.push_back(asset_title);
				}

				break;
				case 2://desc
				{
					COM_ASSET_REQUEST_OBJECT asset_data;
					init_com_asset_request_object(asset_data);
					asset_data.id = 2;
					asset_data.required = 1;
					asset_data.type = NATIVE_ASSETTYPE_DATA;
					asset_data.data.type = ASSET_DATATYPE_DESC;
					asset_data.data.len = imp.native.natreq.desc.len * 3;
					cimp.native.assets.push_back(asset_data);
				}
				break;
				case 3://icon
				{
					COM_ASSET_REQUEST_OBJECT asset_icon;
					init_com_asset_request_object(asset_icon);
					asset_icon.id = 3;
					asset_icon.required = 1;
					asset_icon.type = NATIVE_ASSETTYPE_IMAGE;
					asset_icon.img.type = ASSET_IMAGETYPE_ICON;
					asset_icon.img.h = imp.native.natreq.icon.h;
					asset_icon.img.w = imp.native.natreq.icon.w;
					transform_native_mimes(imp.native.natreq.icon.mimes, asset_icon.img.mimes);
					cimp.native.assets.push_back(asset_icon);
				}

				break;
				case 4://img
				{
					COM_ASSET_REQUEST_OBJECT asset_img;
					init_com_asset_request_object(asset_img);
					asset_img.id = 4;
					asset_img.required = 1;
					asset_img.type = NATIVE_ASSETTYPE_IMAGE;
					asset_img.img.type = ASSET_IMAGETYPE_MAIN;
					asset_img.img.h = imp.native.natreq.img.h;
					asset_img.img.w = imp.native.natreq.img.w;
					// imp.native.natreq.img.mimes 现在不知道，先不填充
					transform_native_mimes(imp.native.natreq.img.mimes, asset_img.img.mimes);
					cimp.native.assets.push_back(asset_img);
				}

				break;
				}
			}
		}
		cimp.bidfloor = imp.bidfloor;

		cimp.bidfloorcur = "CNY";

		if (imp.instl == 2)
		{
			cimp.ext.instl = INSTL_INSERT;
		}
		else if (imp.instl == 3)
		{
			cimp.ext.instl = INSTL_FULL;
		}
		c.imp.push_back(cimp);

		break;   // 只取出第一个  
	}

	//app
	c.app.id = m.app.id;
	c.app.name = m.app.name;
	// 现在没有分类表，不能转换
	for (i = 0; i < m.app.cat.size(); i++)
	{
		vector<uint32_t> &v_cat = app_cat_table_adx2com[m.app.cat[i]];

		for (j = 0; j < v_cat.size(); j++)
		{
			c.app.cat.insert(v_cat[j]);
		}
	}
	c.app.bundle = m.app.bundle;

	//device
	c.device.os = OS_UNKNOWN;
	transform(m.device.os.begin(), m.device.os.end(), m.device.os.begin(), ::tolower);
	if (m.device.os == "ios")
	{
		c.device.os = OS_IOS;

		if (m.device.ifa != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA, m.device.ifa));
		}
	}
	else if (m.device.os == "android")
	{
		c.device.os = OS_ANDROID;

		if (m.device.dpidsha1 != "")
		{
			transform(m.device.dpidsha1.begin(), m.device.dpidsha1.end(), m.device.dpidsha1.begin(), ::tolower);
			c.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, m.device.dpidsha1));
		}

		if (m.device.dpidmd5 != "")
		{
			transform(m.device.dpidmd5.begin(), m.device.dpidmd5.end(), m.device.dpidmd5.begin(), ::tolower);
			c.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, m.device.dpidmd5));
		}

		if (m.device.didmd5 != "")
		{
			transform(m.device.didmd5.begin(), m.device.didmd5.end(), m.device.didmd5.begin(), ::tolower);
			c.device.dids.insert(make_pair(DID_IMEI_MD5, m.device.didmd5));
		}
		if (m.device.didsha1 != "")
		{
			transform(m.device.didsha1.begin(), m.device.didsha1.end(), m.device.didsha1.begin(), ::tolower);
			c.device.dids.insert(make_pair(DID_IMEI_SHA1, m.device.didsha1));
		}
	}

	if (m.device.mac != "")
	{
		transform(m.device.mac.begin(), m.device.mac.end(), m.device.mac.begin(), ::tolower);
		c.device.dids.insert(make_pair(DID_MAC, m.device.mac));
	}
	if (m.device.macmd5 != "")
	{
		transform(m.device.macmd5.begin(), m.device.macmd5.end(), m.device.macmd5.begin(), ::tolower);
		c.device.dids.insert(make_pair(DID_MAC_MD5, m.device.macmd5));
	}
	if (m.device.macsha1 != "")
	{
		transform(m.device.macsha1.begin(), m.device.macsha1.end(), m.device.macsha1.begin(), ::tolower);
		c.device.dids.insert(make_pair(DID_MAC_SHA1, m.device.macsha1));
	}

	c.device.ua = m.device.ua;
	c.device.ip = m.device.ip;
	if (c.device.ip != "")
	{
		c.device.location = getRegionCode(geodb, c.device.ip);
	}

	c.device.geo.lat = m.device.geo.lat;
	c.device.geo.lon = m.device.geo.lon;
	c.device.carrier = transform_carrier(m.device.carrier);
	//	c.device.make = m.device.make;
	string s_make = m.device.make;
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;

		c.device.makestr = s_make;
		for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				c.device.make = it->second;
				break;
			}
		}
	}

	c.device.model = m.device.model;
	//c.device.os
	c.device.osv = m.device.osv;
	c.device.connectiontype = transform_connectiontype(m.device.connectiontype);
	c.device.devicetype = transform_devicetype(m.device.devicetype);

	// user
	c.user.id = m.user.id;
	c.user.yob = m.user.yob;
	c.user.keywords = m.user.keywords;
	if (m.user.gender == "M")
	{
		c.user.gender = GENDER_MALE;
	}
	else if (m.user.gender == "F")
	{
		c.user.gender = GENDER_FEMALE;
	}
	else
		c.user.gender = GENDER_UNKNOWN;

	//cur
	for (i = 0; i < m.cur.size(); i++)
	{
		c.cur.insert(m.cur[i]);
	}

	//bcat
	for (i = 0; i < m.bcat.size(); i++)
	{
		vector<uint32_t> &v_cat = adv_cat_table_adx2com[m.bcat[i]];
		for (j = 0; j < v_cat.size(); j++)
		{
			c.bcat.insert(v_cat[j]);
		}
	}
}

void transform_video_to_json(IN VIDEORESOBJECT &video, OUT string &adm)
{
	char *text = NULL;
	json_t *root = NULL;

	root = json_new_object();
	if (root == NULL)
		return;

	jsonInsertString(root, "src", video.src.c_str());
	jsonInsertInt(root, "duration", video.duration);
	jsonInsertString(root, "landing", video.landing.c_str());

	json_t *array = json_new_array();
	json_t *label;
	for (int i = 0; i < video.starttrackers.size(); ++i)
	{
		label = json_new_string(video.starttrackers[i].c_str());
		json_insert_child(array, label);
	}
	json_insert_pair_into_object(root, "starttrackers", array);

	array = json_new_array();
	for (int i = 0; i < video.completetrackers.size(); ++i)
	{
		label = json_new_string(video.completetrackers[i].c_str());
		json_insert_child(array, label);
	}
	json_insert_pair_into_object(root, "completetrackers", array);

	array = json_new_array();
	for (int i = 0; i < video.clicktrackers.size(); ++i)
	{
		label = json_new_string(video.clicktrackers[i].c_str());
		json_insert_child(array, label);
	}
	json_insert_pair_into_object(root, "clicktrackers", array);

	json_tree_to_string(root, &text);
	json_free_value(&root);

	if (text == NULL)
	{
		return;
	}
	adm = text;
	free(text);
	serial_json("\"", "\\\"", adm);
}

void transform_native_to_json(IN NATIVERESOBJECT &native, OUT string &adm)
{
	char *text = NULL;
	json_t *root = NULL;

	root = json_new_object();
	if (root == NULL)
		return;

	jsonInsertString(root, "title", native.title.c_str());
	jsonInsertString(root, "desc", native.desc.c_str());
	jsonInsertString(root, "icon", native.icon.c_str());
	jsonInsertString(root, "img", native.img.c_str());
	jsonInsertString(root, "landing", native.landing.c_str());

	json_t *array = json_new_array();
	json_t *label;
	for (int i = 0; i < native.imptrackers.size(); ++i)
	{
		label = json_new_string(native.imptrackers[i].c_str());
		json_insert_child(array, label);
	}
	json_insert_pair_into_object(root, "imptrackers", array);

	array = json_new_array();
	for (int i = 0; i < native.clicktrackers.size(); ++i)
	{
		label = json_new_string(native.clicktrackers[i].c_str());
		json_insert_child(array, label);
	}
	json_insert_pair_into_object(root, "clicktrackers", array);

	json_tree_to_string(root, &text);
	json_free_value(&root);

	if (text == NULL)
	{
		return;
	}
	adm = text;
	free(text);
	serial_json("\"", "\\\"", adm);
}

void transform_response(IN COM_RESPONSE &c, IN ADXTEMPLATE &adxtemp, IN COM_REQUEST &crequest, OUT MESSAGERESPONSE &m)
{

	string trace_url_par = string("&") + get_trace_url_par(crequest, c);
	m.id = c.id;	
	for (int i = 0; i < c.seatbid.size(); i++)

	{
		SEATBIDOBJECT seatbid;
		COM_SEATBIDOBJECT &c_seatbid = c.seatbid[i];

		for (int j = 0; j < c_seatbid.bid.size(); j++)
		{
			BIDOBJECT bid;
			init_bid_object(bid);
			COM_BIDOBJECT &c_bid = c_seatbid.bid[j];
			bid.id = c_bid.mapid;
			bid.impid = c_bid.impid;
			bid.price = c_bid.price;// 转换
			//replace_https_url(adxtemp,crequest.is_secure);

			if ((bid.nurl = adxtemp.nurl) != "")
			{
                bid.nurl += trace_url_par;
			}
			string iurl;
			if (adxtemp.iurl.size() > 0)
			{
				iurl = adxtemp.iurl[0] + trace_url_par;
			}

			string curl = c_bid.curl;

			string cturl;
			if (adxtemp.cturl.size() > 0)
			{
				if (c_bid.redirect == 1)
				{
					string curl_temp = curl;

					int en_curl_temp_len = 0;

					char *en_curl_temp = urlencode(curl_temp.c_str(), curl_temp.size(), &en_curl_temp_len);
					string temp_curl = adxtemp.cturl[0] + trace_url_par;

					curl = temp_curl + "&curl=" + string(en_curl_temp, en_curl_temp_len);

					cturl = "";

					free(en_curl_temp);
				}
				else
				{
                    cturl = adxtemp.cturl[0] + trace_url_par;
				}
			}

			string adm;
			if (crequest.imp[0].type == IMP_BANNER)
			{
				for (int k = 0; k < adxtemp.adms.size(); k++)
				{
					if ((adxtemp.adms[k].os == crequest.device.os) && (adxtemp.adms[k].type == c_bid.type))
					{
						adm = adxtemp.adms[k].adm;

						if (adm != "")
						{
							string cmonitorurl = "";

							if (curl != "")
							{
								replaceMacro(adm, "#CURL#", curl.c_str());
							}

							if (cturl != "")//redirect为1时，cturl不替换"#CTURL#"
							{
								replaceMacro(adm, "#CTURL#", cturl.c_str());
							}

							if (c_bid.sourceurl != "")
							{
								replaceMacro(adm, "#SOURCEURL#", c_bid.sourceurl.c_str());
							}

							if (c_bid.monitorcode != "")
							{
								string moncode = c_bid.monitorcode;

								replaceMacro(adm, "#MONITORCODE#", moncode.c_str());
							}

							if (c_bid.cmonitorurl.size() > 0)
							{
								cmonitorurl = c_bid.cmonitorurl[0];
								replaceMacro(adm, "#CMONITORURL#", cmonitorurl.c_str());
							}

							replaceMacro(adm, "#IURL#", iurl.c_str());

							bid.adm = adm;
						}

						break;
					}
				}

			}
			else if (crequest.imp[0].type == IMP_VIDEO)
			{
				bid.video.src = c_bid.sourceurl;
				bid.video.duration = 5;  // 先设定为默认值
				bid.video.landing = curl;

				bid.video.starttrackers.push_back(iurl);
				for (int k = 0; k < c_bid.imonitorurl.size(); ++k)
					bid.video.starttrackers.push_back(c_bid.imonitorurl[k]);

				if (adxtemp.aurl != "")
				{
					string aurl = adxtemp.aurl + trace_url_par;
					bid.video.completetrackers.push_back(aurl);
				}

				//bid.video.completetrackers
				if (c_bid.redirect != 1)
					bid.video.clicktrackers.push_back(cturl);
				for (int k = 0; k < c_bid.cmonitorurl.size(); ++k)
					bid.video.clicktrackers.push_back(c_bid.cmonitorurl[k]);
				transform_video_to_json(bid.video, adm);
				bid.adm = adm;
			}
			else if (crequest.imp[0].type == IMP_NATIVE)
			{
				for (uint32_t k = 0; k < c_bid.native.assets.size(); ++k)
				{
					COM_ASSET_RESPONSE_OBJECT &asset = c_bid.native.assets[k];
					if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						bid.native.title = asset.title.text;
					}
					else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						if (asset.id == 3)  // icon
							bid.native.icon = asset.img.url;
						else
							bid.native.img = asset.img.url;
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						bid.native.desc = asset.data.value;
					}
				}
				bid.native.landing = curl;
				if (c_bid.redirect != 1)
					bid.native.clicktrackers.push_back(cturl);
				for (int k = 0; k < c_bid.cmonitorurl.size(); ++k)
					bid.native.clicktrackers.push_back(c_bid.cmonitorurl[k]);

				bid.native.imptrackers.push_back(iurl);
				for (int k = 0; k < c_bid.imonitorurl.size(); ++k)
					bid.native.imptrackers.push_back(c_bid.imonitorurl[k]);
				transform_native_to_json(bid.native, adm);
				bid.adm = adm;
			}

			seatbid.bid.push_back(bid);
		}
		m.seatbid.push_back(seatbid);
	}

	m.bidid = c.bidid;

	m.cur = "CNY";
}

static bool callback_process_crequest(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = adxtemplate.ratio;

	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
		return false;

	for (int i = 0; i < crequest.imp.size(); i++)
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

	if ((ratio > -0.000001) && (ratio < 0.000001))
	{
		cflog(g_logid_local, LOGERROR, "callback_check_price failed! ratio:%f", ratio);

		return false;
	}

	if ((price - crequest.imp[0].bidfloor) >= 0.000001)
		return true;
	else
		return false;
}

static bool callback_check_exts(IN const COM_REQUEST &crequest, IN const map<uint8_t, string> &exts)
{
	return true;
}

int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata,
	OUT string &requestid, OUT string &response_short, OUT FLOW_BID_INFO &flow_bid)
{
	int errcode = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	MESSAGEREQUEST mrequest;//adx msg request
	COM_REQUEST crequest;//common msg request
	MESSAGERESPONSE mresponse;
	COM_RESPONSE cresponse;
	ADXTEMPLATE adxtemp;
	int imptype = 0;
	string output = "";
	string str_ferrcode = "";
	timespec ts1, ts2;
	long lasttime = 0;
	getTime(&ts1);

	if ((recvdata == NULL) || (ctx == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata or ctx is null");
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata->data is null or recvdata->length is 0");
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	init_message_request(mrequest);
	if (!parse_iflytek_request(recvdata->data, mrequest, imptype))
	{
		cflog(g_logid_local, LOGERROR, "parse_iflytek_request failed! Mark errcode: E_BAD_REQUEST");
		errcode = E_BAD_REQUEST;
		goto exit;
	}

	requestid = mrequest.id;

	init_com_message_request(&crequest);
	transform_request(mrequest, imptype, crequest);
	flow_bid.set_req(crequest);

	if (crequest.id == "")
	{
		errcode = E_REQUEST_NO_BID;
		goto exit;
	}

	if (crequest.imp.size() == 0)
	{
		errcode = E_REQUEST_IMP_SIZE_ZERO;
		goto exit;
	}
	if (imptype == IMP_UNKNOWN)
	{
		errcode = E_REQUEST_IMP_TYPE;
		goto exit;
	}
	if (crequest.device.dids.size() == 0 && crequest.device.dpids.size() == 0)
	{
		errcode = E_REQUEST_DEVICE_ID;
		goto exit;
	}

	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		errcode = E_REQUEST_DEVICE_IP;
		goto exit;
	}

	init_com_message_response(&cresponse);
	init_com_template_object(&adxtemp);
	ferrcode = bid_filter_response(ctx, index, crequest, callback_process_crequest, NULL, callback_check_price, NULL, &cresponse, &adxtemp);
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	errcode = ferrcode.errcode;
	flow_bid.set_bid_flow(ferrcode);
	g_ser_log.send_bid_log(index, crequest, cresponse, errcode);

exit:
	if (errcode == E_SUCCESS)
	{
		init_message_response(mresponse);
		transform_response(cresponse, adxtemp, crequest, mresponse);

		setJsonResponse(mresponse, output);

		senddata = string("Content-Type: application/json\r\nx-openrtb-version: 2.0\r\nContent-Length: ")
			+ intToString(output.size()) + "\r\n\r\n" + output;

		response_short =
			cresponse.bidid + string(",") +
			cresponse.seatbid[0].bid[0].mapid + string(",") +
			crequest.device.deviceid + string(",") +
			intToString(crequest.device.deviceidtype) + string(",") +
			doubleToString(cresponse.seatbid[0].bid[0].price);
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());
	va_cout("%s,spent time:%ld,err:0x%x,desc:%s", crequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());

	flow_bid.set_err(errcode);
	return errcode;
}
