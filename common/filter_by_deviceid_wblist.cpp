#include <string>
#include <time.h>
#include <vector>
#include <map>
#include <set>
#include <assert.h>
#include "type.h"
#include "util.h"
#include "errorcode.h"
#include "setlog.h"
#include "filter_by_deviceid_wblist.h"


static string make_bllist_key(uint8_t type)
{
	string deviceid_bl = "";

	switch (type)
	{
	case DID_IMEI:
		deviceid_bl = "dsp_bl_imei";
		break;

	case DID_IMEI_SHA1:
		deviceid_bl = "dsp_bl_sha1_imei";
		break;

	case DID_IMEI_MD5:
		deviceid_bl = "dsp_bl_md5_imei";
		break;

	case DID_MAC:
		deviceid_bl = "dsp_bl_mac";
		break;

	case DID_MAC_SHA1:
		deviceid_bl = "dsp_bl_sha1_mac";
		break;

	case DID_MAC_MD5:
		deviceid_bl = "dsp_bl_md5_mac";
		break;

	case DPID_ANDROIDID:
		deviceid_bl = "dsp_bl_androidid";
		break;

	case DPID_ANDROIDID_SHA1:
		deviceid_bl = "dsp_bl_sha1_androidid";
		break;

	case DPID_ANDROIDID_MD5:
		deviceid_bl = "dsp_bl_md5_androidid";
		break;

	case DPID_IDFA:
		deviceid_bl = "dsp_bl_idfa";
		break;

	case DPID_IDFA_SHA1:
		deviceid_bl = "dsp_bl_sha1_idfa";
		break;

	case DPID_IDFA_MD5:
		deviceid_bl = "dsp_bl_md5_idfa";
		break;

	default:
		break;
	}

	return deviceid_bl;
}

static string make_wbdetaillist_key(uint8_t type, string relationid, bool is_wlist)
{
	string deviceid_wl_relationid = "";

	switch (type)
	{
	case DID_OTHER:// == DPID_OTHER
		deviceid_wl_relationid = "other_";
		break;

	case DID_IMEI:
		deviceid_wl_relationid = "imei_";
		break;

	case DID_IMEI_SHA1:
		deviceid_wl_relationid = "imei_sha1_";
		break;

	case DID_IMEI_MD5:
		deviceid_wl_relationid = "imei_md5_";
		break;

	case DID_MAC:
		deviceid_wl_relationid = "mac_";
		break;

	case DID_MAC_SHA1:
		deviceid_wl_relationid = "mac_sha1_";
		break;

	case DID_MAC_MD5:
		deviceid_wl_relationid = "mac_md5_";
		break;

	case DPID_ANDROIDID:
		deviceid_wl_relationid = "androidid_";
		break;

	case DPID_ANDROIDID_SHA1:
		deviceid_wl_relationid = "androidid_sha1_";
		break;

	case DPID_ANDROIDID_MD5:
		deviceid_wl_relationid = "androidid_md5_";
		break;

	case DPID_IDFA:
		deviceid_wl_relationid = "idfa_";
		break;

	case DPID_IDFA_SHA1:
		deviceid_wl_relationid = "idfa_sha1_";
		break;

	case DPID_IDFA_MD5:
		deviceid_wl_relationid = "idfa_md5_";
		break;

	default:
		break;
	}

	if (is_wlist)
	{
		deviceid_wl_relationid += "wl_";
	}
	else
	{
		deviceid_wl_relationid += "bl_";
	}

	if (deviceid_wl_relationid != "")
	{
		deviceid_wl_relationid += relationid;
	}

	return deviceid_wl_relationid;
}

// 过滤黑名单
int filter_check_blist(IN uint64_t logid, IN const COM_REQUEST &crequest, IN redisContext *redis_wblist_detail, IN bool is_print_time)
{
	const char *id = crequest.id.c_str();
	int bexist = E_UNKNOWN;
	timespec ts1;

	vector<map<uint8_t, string> > v_ids;
	v_ids.push_back(crequest.device.dpids);
	v_ids.push_back(crequest.device.dids);

	for (int i = 0; i < v_ids.size(); i++)//dpids优先
	{
		const map<uint8_t, string> &ids = v_ids[i];

		map<uint8_t, string>::const_reverse_iterator crit;
		for (crit = ids.rbegin(); crit != ids.rend(); ++crit)//map默认升序排序，故降序查找type
		{
			const uint8_t &first = crit->first;
			const string &second = crit->second;
			{
				string _deviceid_hash_bl = make_bllist_key(first); //groupid

				cflog(logid, LOGINFO, "%s,bl_list_key:%s", id, _deviceid_hash_bl.c_str());
				getTime(&ts1);
				if (_deviceid_hash_bl != "")
				{
					bexist = existRedisSetKey(redis_wblist_detail, logid, _deviceid_hash_bl, second);
					record_cost(logid, is_print_time, id, "existRedisSetKey bl", ts1);

					if (bexist == E_SUCCESS)
					{
						cflog(logid, LOGINFO, "%s,find deviceid:%s in redis deviceid_hash_bl:%s", id, second.c_str(), _deviceid_hash_bl.c_str());
						goto exit;
					}
					else if (bexist == E_REDIS_CONNECT_INVALID)
					{
						cflog(logid, LOGDEBUG, "read wlist redis context invalid!");
						goto exit;
					}
				}
			}
		}
	}
exit:
	return bexist;
}

int filter_check_wblist(IN uint64_t logid, IN const COM_REQUEST &crequest, IN redisContext *redis_wblist_detail,
					   IN const WBLIST &wblist, IN string groupid, IN bool is_print_time, IN bool is_wlist,
					   OUT uint8_t &deviceidtype, OUT string &deviceid)
{
	const char *id = crequest.id.c_str();
	int bexist = E_UNKNOWN; // 未设置黑名单（白名单）
	timespec ts1;

	vector<map<uint8_t, string> > v_ids;

	if (wblist.relationid == "")
	{
		cflog(logid, LOGDEBUG, "%s,groupid:%s relationid is empty!", id, groupid.c_str());
		goto exit;
	}

	v_ids.push_back(crequest.device.dpids);
	v_ids.push_back(crequest.device.dids);

	for (int i = 0; i < v_ids.size(); i++)//dpids优先
	{
		const map<uint8_t, string> &ids = v_ids[i];

		map<uint8_t, string>::const_reverse_iterator crit;
		for (crit = ids.rbegin(); crit != ids.rend(); ++crit)//map默认升序排序，故降序查找type
		{
			const uint8_t &first = crit->first;
			const string &second = crit->second;

			const set<uint8_t> &wb_set = is_wlist ? wblist.whitelist : wblist.blacklist;
			set<uint8_t>::iterator s_wlist_it = wb_set.find(first);

			if (s_wlist_it == wb_set.end())
			{
				continue;
			}

			string _deviceid_hash_wbl_relationid = make_wbdetaillist_key(first, wblist.relationid, is_wlist);

			cflog(logid, LOGINFO, "%s, _deviceid_hash_wbl_relationid:%s", id, _deviceid_hash_wbl_relationid.c_str());

			if (_deviceid_hash_wbl_relationid != "")
			{
				getTime(&ts1);
				bexist = existRedisSetKey(redis_wblist_detail, logid, _deviceid_hash_wbl_relationid, second);
				record_cost(logid, is_print_time, id, "existRedisSetKey", ts1);
				if (bexist == E_SUCCESS)
				{
					cflog(logid, LOGINFO, "%s,find deviceid:%s in redis deviceid_hash_wbl_relationid: %s",
						id, second.c_str(), _deviceid_hash_wbl_relationid.c_str());

					deviceidtype = first;
					deviceid = second;
					goto exit;
				}
				else if (bexist == E_REDIS_CONNECT_INVALID)
				{
					cflog(logid, LOGDEBUG, "read wblist redis context invalid!");
					goto exit;
				}
				else
				{
					cflog(logid, LOGWARNING, "%s,can't find deviceid:%s in redis deviceid_hash_wbl_relationid:%s",
						id, second.c_str(), _deviceid_hash_wbl_relationid.c_str());
				}
			}
		}
	}

exit:
	return bexist;
}

int filter_group_by_wlist(IN uint64_t logid, IN REDIS_INFO *redis_cashe,
						  IN redisContext *redis_wblist_detail, IN const COM_REQUEST &crequest,
						  IN const vector<pair<string, GROUPINFO> > &v_p_groupinfo_first_temp,
						  OUT vector<pair<string, GROUPINFO> > &v_p_groupinfo_first, OUT map<string, bool> &has_blist,
						  OUT uint8_t &deviceidtype, OUT string &deviceid,
						  INOUT map<string, ERRCODE_GROUP> &m_err_group, IN bool is_print_time)
{
	int ret = E_SUCCESS;
	const char *id = crequest.id.c_str();
	map<string, WBLIST> &wblist = redis_cashe->wblist;

	cflog(logid, LOGINFO, "--enter %s/%d...redis_cashe->wblist.size=%d", __FUNCTION__, __LINE__, wblist.size());

	if (redis_wblist_detail == NULL)
	{
		ret = E_REDIS_CONNECT_INVALID;
		cflog(logid, LOGDEBUG, "%s,redis_wblist_detail context is null", crequest.id.c_str());
		goto exit;
	}

	for (int i = 0; i < v_p_groupinfo_first_temp.size(); i++)
	{
		const string &groupid = v_p_groupinfo_first_temp[i].first;

		map<string, WBLIST>::iterator m_wblist_it = wblist.find(groupid);
		if (m_wblist_it == wblist.end())
		{
			ERRCODE_GROUP err_group = { 0 };
			err_group.errcode = E_CACHE;
			m_err_group.insert(make_pair(groupid, err_group));
			writeLog(logid, LOGDEBUG, "m_wblist_it == wblist.end(), groupid:%s", groupid.c_str());
			continue;
		}

		WBLIST &wblist_temp = m_wblist_it->second;
		bool is_black = wblist_temp.blacklist.size() > 0;
		bool is_white = wblist_temp.whitelist.size() > 0;

		if (is_black == is_white)
		{
			ERRCODE_GROUP err_group = { 0 };
			err_group.errcode = E_CACHE;
			m_err_group.insert(make_pair(groupid, err_group));
			writeLog(logid, LOGDEBUG, "is_black == is_white, groupid:%s", groupid.c_str());
			continue;
		}

		if (is_black){
			has_blist[groupid] = true;
		}

		// 总黑名单  （以前的代码）
		ret = filter_check_blist(logid, crequest, redis_wblist_detail, is_print_time);
		if (ret == E_SUCCESS)  //find deviceid in blcak list
		{
			ERRCODE_GROUP err_group = { 0 };

			err_group.errcode = E_FILTER_GROUP_ALLBL;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}
		else if (ret == E_REDIS_CONNECT_INVALID)
		{
			goto exit;
		}

		// 检查
		ret = filter_check_wblist(logid, crequest, redis_wblist_detail,
			wblist_temp, groupid, is_print_time, is_white,
			deviceidtype, deviceid);

		if (ret == E_REDIS_CONNECT_INVALID){
			goto exit;
		}

		if (ret == E_SUCCESS)
		{
			if (is_white){
				v_p_groupinfo_first.push_back(v_p_groupinfo_first_temp[i]);
			}
			else
			{
				ERRCODE_GROUP err_group = { 0 };
				err_group.errcode = E_FILTER_GROUP_IN_BL;
				m_err_group.insert(make_pair(groupid, err_group));
			}
		}
		else
		{
			if (is_black){
				v_p_groupinfo_first.push_back(v_p_groupinfo_first_temp[i]);
			}
			else
			{
				ERRCODE_GROUP err_group = { 0 };
				err_group.errcode = E_FILTER_GROUP_NOT_WL;
				m_err_group.insert(make_pair(groupid, err_group));
			}
		}
	}

	if (v_p_groupinfo_first.size() == 0){
		ret = E_FILTER_GROUP_WBL;
	}
	else
	{
		ret = E_SUCCESS;
		if (deviceidtype == DPID_UNKNOWN)
		{
			map<uint8_t, string>::const_iterator it;
			if (crequest.device.dpids.size() > 0)
			{
				it = crequest.device.dpids.begin();
				deviceidtype = it->first;
				deviceid = it->second;
			}
			else if (crequest.device.dids.size() > 0)
			{
				it = crequest.device.dids.begin();
				deviceidtype = it->first;
				deviceid = it->second;
			}
		}
	}

exit:
	cflog(logid, LOGINFO, "--leave %s/%d...", __FUNCTION__, __LINE__);
	return ret;
}
