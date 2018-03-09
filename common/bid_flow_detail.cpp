#include <stdlib.h>
#include "json.h"
#include "json_util.h"
#include "errorcode.h"
#include "util.h"
#include "bid_flow_detail.h"


FLOW_BID_INFO::FLOW_BID_INFO(uint8_t _adx) :adx(_adx)
{
}

void FLOW_BID_INFO::reset()
{
#ifdef DETAILLOG
	struct timespec ts;
	getTime(&ts);

	req_id = "";
	req_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	appid = "";
	imp_type = 0;
	width = 0;
	height = 0;
	devicetype = 0;
	os = 0;
	location = 0;
	connectiontype = 0;
	carrier = 0;
	errcode = E_UNKNOWN;
	detail_grp.clear();

	ext = "";
#endif
}

void FLOW_BID_INFO::set_req(const COM_REQUEST &commrequest)
{
#ifdef DETAILLOG
	req_id = commrequest.id;
	appid = commrequest.app.id;

	if (commrequest.imp.size() > 0)
	{
		const COM_IMPRESSIONOBJECT &imp = commrequest.imp[0];
		imp_type = imp.type;
		if (imp_type == IMP_NATIVE)
		{
			const vector<COM_ASSET_REQUEST_OBJECT> &assets = imp.native.assets;
			for (uint32_t i = 0; i < assets.size(); i++)
			{
				if (assets[i].type != NATIVE_ASSETTYPE_IMAGE){
					continue;
				}
				if (assets[i].img.type != ASSET_IMAGETYPE_MAIN){
					continue;
				}
				width = assets[i].img.w;
				height = assets[i].img.h;
				break;
			}
		}
		else if (imp_type == IMP_BANNER)
		{
			width = imp.banner.w;
			height = imp.banner.h;
		}
		else
		{
			width = imp.video.w;
			height = imp.video.h;
		}
	}

	devicetype = commrequest.device.devicetype;
	os = commrequest.device.os;
	location = commrequest.device.location;
	connectiontype = commrequest.device.connectiontype;
	carrier = commrequest.device.carrier;
#endif
}

void FLOW_BID_INFO::set_bid_flow(const FULL_ERRCODE &detail)
{
#ifdef DETAILLOG
	detail_grp = detail.err_group_family[0].m_err_group;

	map<string, ERRCODE_GROUP>::const_iterator it = detail.err_group_family[1].m_err_group.begin();
	for (; it != detail.err_group_family[1].m_err_group.end(); it++)
	{
		detail_grp.insert(*it);
	}
#endif
}

void FLOW_BID_INFO::set_err(int err)
{
#ifdef DETAILLOG
	errcode = err;
#endif
}

void FLOW_BID_INFO::set_ext(json_t *ext_info)
{
#ifdef DETAILLOG
	if (!ext_info){
		return;
	}

	char *text = NULL;
	json_tree_to_string(ext_info, &text);

	if (text != NULL)
	{
		ext = text;
		free(text);
		text = NULL;
	}
#endif
}

void FLOW_BID_INFO::add_errcode(json_t *json, int err)const
{
	char buf[64] = {0};
	if (err == 0){
		buf[0] = '0';
	}
	else{
		sprintf(buf, "0x%08X", err);
	}
	jsonInsertString(json, "nbrcode", buf);
}

string FLOW_BID_INFO::to_string()const
{
	string ret;
#ifdef DETAILLOG
	char *text = NULL;

	json_t *root = NULL;
	root = json_new_object();
	if (root == NULL){
		return ret;
	}

	jsonInsertString(root, "request Id", req_id.c_str());
	jsonInsertInt64(root, "time", req_time);
	jsonInsertString(root, "adxid", intToString(adx).c_str());
	jsonInsertString(root, "appid", appid.c_str());
	jsonInsertInt(root, "creativeid", imp_type);
	jsonInsertInt(root, "width", width);
	jsonInsertInt(root, "height", height);
	jsonInsertInt(root, "devicetype", devicetype);
	jsonInsertInt(root, "os", os);
	jsonInsertInt(root, "city", location);
	jsonInsertInt(root, "connectiontype", connectiontype);
	jsonInsertInt(root, "carrier", carrier);
	add_errcode(root, errcode);
	json_t *grp = get_grp_detail();
	if (grp){
		json_insert_pair_into_object(root, "AdGroupItems", grp);
	}
	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		ret = text;
		free(text);
		text = NULL;
	}

	json_free_value(&root);
#endif
	return ret;
}

json_t* FLOW_BID_INFO::get_grp_detail()const
{
	if (detail_grp.size() == 0){
		return NULL;
	}

	json_t *arr = NULL;
	arr = json_new_array();
	if (arr == NULL){
		return NULL;
	}

	map<string, ERRCODE_GROUP>::const_iterator it;
	for (it = detail_grp.begin(); it != detail_grp.end(); it++)
	{
		json_t *group = json_new_object();
		json_insert_child(arr, group);

		jsonInsertString(group, "adgroupid", it->first.c_str());
		add_errcode(group, it->second.errcode);
		if (it->second.m_err_creative.size() > 0)
		{
			json_t *creatives = get_creative_detail(it->second.m_err_creative);
			if (creatives)
			{
				json_insert_pair_into_object(group, "mapids", creatives);
			}
		}
	}

	return arr;
}

json_t* FLOW_BID_INFO::get_creative_detail(const map<string, int> &creatives)const
{
	json_t *arr = NULL;
	arr = json_new_array();
	if (arr == NULL){
		return NULL;
	}

	map<string, int>::const_iterator it;
	for (it = creatives.begin(); it != creatives.end(); it++)
	{
		json_t *creative = json_new_object();
		json_insert_child(arr, creative);

		jsonInsertString(creative, "mapid", it->first.c_str());
		add_errcode(creative, it->second);
	}

	return arr;
}
