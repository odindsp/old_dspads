#include <string>
#include <stdlib.h>
#include "util.h"
#include "setlog.h"
#include "json.h"
#include "json_util.h"
#include "errorcode.h"
#include "redisoperation.h"
#include "bid_filter_dump.h"

void dump_vector_combidobject(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<COM_BIDOBJECT> &v_bid)
{
	int i = 0;

	int count = v_bid.size();
	
	ENTER(logid, level, id)

	for(i = 0; i < count; i++)
	{
		cflog(logid, level, "%s, %s/%d, m_v_cbid[%d/%d](price:%f)", id, __FUNCTION__, __LINE__, i, count, v_bid[i].mapid.c_str());
	}

	LEAVE(logid, level, id)
}

void dump_set_regioncode(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint32_t> &s_regioncode)
{
	int i = 0;

	set<uint32_t>::const_iterator s_regioncode_it = s_regioncode.begin();

	ENTER(logid, level, id)

	for(s_regioncode_it; s_regioncode_it != s_regioncode.end(); s_regioncode_it++)
	{
		cflog(logid, level, "%s, %s/%d, regioncode[%d]=%d", id, __FUNCTION__, __LINE__, i++, *s_regioncode_it);
	}

	LEAVE(logid, level, id)
}

void dump_set_cat(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint32_t> &s_cat)
{
	int i = 0;

	set<uint32_t>::const_iterator s_cat_it = s_cat.begin();

	ENTER(logid, level, id)

	for(s_cat_it; s_cat_it != s_cat.end(); s_cat_it++)
	{
		cflog(logid, level, "%s, %s/%d, cat[%d]=0x%x", id, __FUNCTION__, __LINE__, i++, *s_cat_it);
	}

	LEAVE(logid, level, id)
}

void dump_set_adv(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<string> &s_adv)
{
	int i = 0;

	ENTER(logid, level, id)

	set<string>::const_iterator s_adv_it = s_adv.begin();

	for(s_adv_it; s_adv_it != s_adv.end(); s_adv_it++)
	{
		cflog(logid, level, "%s, %s/%d, adv[%d]=%s", id, __FUNCTION__, __LINE__, i++, s_adv_it->c_str());
	}

	LEAVE(logid, level, id)
}

void dump_set_cat_adv(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint32_t> &s_cat, IN const set<string> &s_adv)
{
	int i = 0;

	set<uint32_t>::const_iterator s_cat_it = s_cat.begin();

	ENTER(logid, level, id)

	for(i = 0; s_cat_it != s_cat.end(); s_cat_it++)
	{
		cflog(logid, level, "%s, %s/%d, cat[%d]=0x%x", id, __FUNCTION__, __LINE__, i++, *s_cat_it);
	}

	set<string>::const_iterator s_adv_it = s_adv.begin();
	for(i = 0; s_adv_it != s_adv.end(); s_adv_it++)
	{
		cflog(logid, level, "%s, %s/%d, adv[%d]=%s", id, __FUNCTION__, __LINE__, i++, s_adv_it->c_str());
	}

	LEAVE(logid, level, id)
}

void dump_set_connectiontype(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_connectiontype)
{
	int i = 0;

	set<uint8_t>::const_iterator s_connectiontype_it = s_connectiontype.begin();

	ENTER(logid, level, id)

	for(s_connectiontype_it; s_connectiontype_it != s_connectiontype.end(); s_connectiontype_it++)
	{
		cflog(logid, level, "%s, %s/%d, connectiontype[%d]=%d", id, __FUNCTION__, __LINE__, i++, *s_connectiontype_it);
	}

	LEAVE(logid, level, id)
}

void dump_set_os(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_os)
{
	int i = 0;

	set<uint8_t>::const_iterator s_os_it = s_os.begin();

	ENTER(logid, level, id)

	for(s_os_it; s_os_it != s_os.end(); s_os_it++)
	{
		cflog(logid, level, "%s, %s/%d, os[%d]=%d", id, __FUNCTION__, __LINE__, i++, *s_os_it);
	}

	LEAVE(logid, level, id)
}

void dump_set_carrier(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_carrier)
{
	int i = 0;

	set<uint8_t>::const_iterator s_carrier_it = s_carrier.begin();

	ENTER(logid, level, id)

	for(s_carrier_it; s_carrier_it != s_carrier.end(); s_carrier_it++)
	{
		cflog(logid, level, "%s, %s/%d, carrier[%d]=%d", id, __FUNCTION__, __LINE__, i++, *s_carrier_it);
	}

	LEAVE(logid, level, id)
}

void dump_set_devicetype(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_devicetype)
{
	int i = 0;

	set<uint8_t>::const_iterator s_devicetype_it = s_devicetype.begin();

	ENTER(logid, level, id)

	for(s_devicetype_it; s_devicetype_it != s_devicetype.end(); s_devicetype_it++)
	{
		cflog(logid, level, "%s, %s/%d, devicetype[%d]=%d", id, __FUNCTION__, __LINE__, i++, *s_devicetype_it);
	}

	LEAVE(logid, level, id)
}

void dump_set_make(IN uint64_t logid, IN uint8_t level, IN const char *id, IN set<uint16_t> &s_make)
{
	int i = 0;

	set<uint16_t>::iterator s_make_it = s_make.begin();

	cflog(logid, level, "%s, enter fn:%s, ln:%d....", id, __FUNCTION__, __LINE__);

	for(s_make_it; s_make_it != s_make.end(); s_make_it++)
	{
		cflog(logid, level, "%s, fn:%s, ln:%d, make[%d]=%d", id, __FUNCTION__, __LINE__, i++, *s_make_it);
	}

	cflog(logid, level, "%s, --leave %s, ln:%d.", id, __FUNCTION__, __LINE__);
}

void dump_vector_mapid(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<string> &v_mapid)
{
	ENTER(logid, level, id)

	LEAVE(logid, level, id)
}

void dump_map_mapinfo(uint64_t logid, IN uint8_t level, IN const char *id, IN const map<string, CREATIVEOBJECT> &m_creativeinfo)
{
	ENTER(logid, level, id)

	LEAVE(logid, level, id)
}

void dump_vector_groupid(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<string> &v_groupid)
{
	ENTER(logid, level, id)

	LEAVE(logid, level, id)
}

void dump_vector_pair_groupinfo(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<pair<string, GROUPINFO> > &v_p_groupinfo_first)
{
	ENTER(logid, level, id)

	for (int i = 0; i < v_p_groupinfo_first.size(); i++)
	{
		cflog(logid, level, "%s, -v_p_groupinfo_first.groupid[%d]=%s", id, i, v_p_groupinfo_first[i].first.c_str());
	}

	LEAVE(logid, level, id)
}

void dump_map_groupinfo(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const map<string, GROUPINFO> &m_groupinfo)
{
	ENTER(logid, level, id)

	map<string, GROUPINFO>::const_iterator it = m_groupinfo.begin();
	for (int i = 0; i < m_groupinfo.size(); i++)
	{
		cflog(logid, level, "%s, groupid[%d]:%s ", id, i, it->first.c_str());
		cflog(logid, level, "\t%s, domain:%s", id, it->second.adomain.c_str());

		string str_cat = "\tcat: ";
		set<uint32_t>::const_iterator it_cat = it->second.cat.begin();
		for (int j = 0; j < it->second.cat.size(); j++)
		{
			str_cat += string(hexToString(*it_cat) + " / ");
			it_cat++;
		}

		cflog(logid, level, "%s, %s", id, str_cat.c_str());

		it++;
	}

	LEAVE(logid, level, id)
}

void dump_map_grouprc(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const map<string, FREQUENCYCAPPINGRESTRICTION> &m_fc)
{
	ENTER(logid, level, id)

	LEAVE(logid, level, id)
}

void dump_user_restriction(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const USER_FREQUENCY_COUNT &ur)
{
	ENTER(logid, level, id)

	cflog(logid, level, "%s, ur.userid:%s, gp_restrictions.size:%d", id, ur.userid.c_str(), ur.user_group_frequency_count.size());
	int i = 0, j =0;
	map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >::const_iterator p_gr = ur.user_group_frequency_count.begin();
	while (p_gr != ur.user_group_frequency_count.end())
	{
		const map<string, USER_CREATIVE_FREQUENCY_COUNT> &gr = p_gr->second;
		cflog(logid, level, "%s, restrictions[%d].groupid:%s, ct_restrictions.size:%d", id, i, p_gr->first.c_str(), gr.size());

		map<string, USER_CREATIVE_FREQUENCY_COUNT>::const_iterator p_ur = gr.begin();
		j = 0;
		while (p_ur != gr.end())
		{
			const USER_CREATIVE_FREQUENCY_COUNT &ct_restriction = p_ur->second;

			cflog(logid, level, "%s, ct_restriction[%d][%d]:: mapid:%s, imp:%d, starttime:%d", 
				id, i ,j++, p_ur->first.c_str(), ct_restriction.imp, ct_restriction.starttime);
			++p_ur;
		}
		++i;
		++p_gr;
	}

	LEAVE(logid, level, id)
}

string fullerrcode_to_string(IN const FULL_ERRCODE &ferrcode, IN uint8_t level)
{
	string response_out;
	char *text = NULL;
	json_t *root = NULL;
	const string keys[2] = {"wlist", "ordinary"};
	
	root = json_new_object();
	if (root == NULL)
		goto exit;

	jsonInsertString(root, "err_top", hexToString(ferrcode.errcode).c_str());
	for (int i = 0; i < 2; i++)
	{
			const ERRCODE_GROUP_FAMILY &err_group_family = ferrcode.err_group_family[i];

			if ((level <= FERR_LEVEL_GROUP) && (err_group_family.m_err_group.size() > 0))
			{
				json_t *label_err_group_family = json_new_object();

				jsonInsertString(label_err_group_family, "err_family", hexToString(err_group_family.errcode).c_str());
				const map<string, ERRCODE_GROUP> &m_err_group = err_group_family.m_err_group;

				json_t *array_err_group_array =  json_new_array();

				for (map<string, ERRCODE_GROUP>::const_iterator it_m_err_group = m_err_group.begin(); it_m_err_group != m_err_group.end(); it_m_err_group++)
				{
					const string &groupid = it_m_err_group->first;
					const ERRCODE_GROUP &err_group = it_m_err_group->second;

					json_t *entry_err_group_array = json_new_object();

					json_t *entry_err_group = json_new_object();

					jsonInsertString(entry_err_group, "err_group", hexToString(err_group.errcode).c_str());

					if ((level <= FERR_LEVEL_CREATIVE) && (err_group.m_err_creative.size() > 0))
					{
						const map<string, int> &m_err_creative = err_group.m_err_creative;

						json_t *array_err_creative_array =  json_new_array();

						for (map<string, int>::const_iterator it_m_err_creative = m_err_creative.begin(); it_m_err_creative != m_err_creative.end(); it_m_err_creative++)
						{
							const string &mapid = it_m_err_creative->first;
							const int &errcode = it_m_err_creative->second;

							json_t *entry_err_creative = json_new_object();

							jsonInsertString(entry_err_creative, mapid.c_str(), hexToString(errcode).c_str());

							json_insert_child(array_err_creative_array, entry_err_creative);
						}

						json_insert_pair_into_object(entry_err_group, "creatives", array_err_creative_array);
					}

					json_insert_pair_into_object(entry_err_group_array, groupid.c_str(), entry_err_group);

					json_insert_child(array_err_group_array, entry_err_group_array);
				}

				json_insert_pair_into_object(label_err_group_family, "groups", array_err_group_array);
				json_insert_pair_into_object(root, keys[i].c_str(), label_err_group_family);
			}
	}	


	json_tree_to_string(root, &text);
  
  if (text != NULL)
  {
	 response_out = text;
	 free(text);
	 text = NULL;
	}

	json_free_value(&root);

exit:
	return response_out;
}

string bid_detail_record(int secends, int interval, const FULL_ERRCODE &ferrcode)
{
	static time_t t_last = time(0);
	static int count = 0;
	static pthread_mutex_t bid_detail_mutex = PTHREAD_MUTEX_INITIALIZER;
	time_t t_now = time(0);
	string str_ferrcode;
	uint8_t err_level = ferrcode.level;
	count++;
	pthread_mutex_lock(&bid_detail_mutex);
	if (count >= interval || t_now - t_last > secends)
	{
		count = 0;
		t_last = t_now;
		err_level = FERR_LEVEL_CREATIVE;
	}
	pthread_mutex_unlock(&bid_detail_mutex);

	str_ferrcode = fullerrcode_to_string(ferrcode, err_level);

	return str_ferrcode;
}

void add_creative_info(REDIS_INFO *redis_cashe, string creativeid, json_t *creative)
{
	map<string, CREATIVEOBJECT>::iterator m_creative_it = redis_cashe->creativeinfo.find(creativeid);

	if (m_creative_it == redis_cashe->creativeinfo.end()){
		return;
	}

	const CREATIVEOBJECT &cinfo = m_creative_it->second;
	jsonInsertString(creative, "groupid_type", cinfo.groupid_type.c_str());
	jsonInsertDouble(creative, "price", cinfo.price);
	jsonInsertString(creative, "adid", cinfo.adid.c_str());
	jsonInsertInt(creative, "type", cinfo.type);
	jsonInsertInt(creative, "ftype", cinfo.ftype);
	jsonInsertInt(creative, "ctype", cinfo.ctype);
	jsonInsertString(creative, "bundle", cinfo.bundle.c_str());
	jsonInsertString(creative, "apkname", cinfo.apkname.c_str());
	jsonInsertInt(creative, "width", cinfo.w);
	jsonInsertInt(creative, "height", cinfo.h);
	jsonInsertString(creative, "curl", cinfo.curl.c_str());
	jsonInsertString(creative, "landingurl", cinfo.landingurl.c_str());
	jsonInsertString(creative, "securecurl", cinfo.securecurl.c_str());
	jsonInsertString(creative, "monitorcode", cinfo.monitorcode.c_str());
	jsonInsertString(creative, "deeplink", cinfo.deeplink.c_str());
	
	// cmonitorurl
	json_t *cmonitorurl = json_new_array();
	json_insert_pair_into_object(creative, "cmonitorurl", cmonitorurl);
	for (size_t i = 0; i < cinfo.cmonitorurl.size(); i++)
	{
		json_t *one = json_new_string(cinfo.cmonitorurl[i].c_str());
		json_insert_child(cmonitorurl, one);
	}

	// imonitorurl
	json_t *imonitorurl = json_new_array();
	json_insert_pair_into_object(creative, "imonitorurl", imonitorurl);
	for (size_t i = 0; i < cinfo.imonitorurl.size(); i++)
	{
		json_t *one = json_new_string(cinfo.imonitorurl[i].c_str());
		json_insert_child(imonitorurl, one);
	}

	jsonInsertString(creative, "sourceurl", cinfo.sourceurl.c_str());
	jsonInsertString(creative, "cid", cinfo.cid.c_str());
	jsonInsertString(creative, "crid", cinfo.crid.c_str());

	// attr
	json_t *attr = json_new_array();
	json_insert_pair_into_object(creative, "attr", attr);
	for (set<uint8_t>::const_iterator it = cinfo.attr.begin(); it != cinfo.attr.end(); it++)
	{
		json_t *one = json_new_number(intToString(*it).c_str());
		json_insert_child(attr, one);
	}

	// badx
	json_t *badx = json_new_array();
	json_insert_pair_into_object(creative, "badx", badx);
	for (set<uint8_t>::const_iterator it = cinfo.badx.begin(); it != cinfo.badx.end(); it++)
	{
		json_t *one = json_new_number(intToString(*it).c_str());
		json_insert_child(badx, one);
	}

	// icon
	json_t *icon = json_new_object();
	json_insert_pair_into_object(creative, "icon", icon);
	jsonInsertInt(icon, "ftype", cinfo.icon.ftype);
	jsonInsertInt(icon, "w", cinfo.icon.w);
	jsonInsertInt(icon, "h", cinfo.icon.h);
	jsonInsertString(icon, "sourceurl", cinfo.icon.sourceurl.c_str());

	// imgs
	json_t *imgs = json_new_array();
	json_insert_pair_into_object(creative, "imgs", imgs);
	for (vector<IMAG_OBJECT>::const_iterator it = cinfo.imgs.begin(); it != cinfo.imgs.end(); it++)
	{
		json_t *one = json_new_object();
		json_insert_child(imgs, one);
		jsonInsertInt(one, "ftype", it->ftype);
		jsonInsertInt(one, "w", it->w);
		jsonInsertInt(one, "h", it->h);
		jsonInsertString(one, "sourceurl", it->sourceurl.c_str());
	}

	jsonInsertString(creative, "title", cinfo.title.c_str());
	jsonInsertString(creative, "description", cinfo.description.c_str());
	jsonInsertInt(creative, "rating", cinfo.rating);
	jsonInsertString(creative, "ctatext", cinfo.ctatext.c_str());

	// ext
	json_t *ext = json_new_object();
	json_insert_pair_into_object(creative, "ext", ext);
	jsonInsertString(ext, "id", cinfo.ext.id.c_str());
	json_t *extinfo = NULL;
	json_parse_document(&extinfo, cinfo.ext.ext.c_str());
	if (extinfo){
		json_insert_pair_into_object(ext, "ext", extinfo);
	}
	else{
		jsonInsertString(ext, "ext", cinfo.ext.ext.c_str());
	}
	
}

void add_grp_info(REDIS_INFO *redis_cashe, string groupid, json_t *group, const GROUPINFO &ginfo)
{
	json_t *cat = json_new_array();
	json_insert_pair_into_object(group, "cat", cat);
	for (set<uint32_t>::const_iterator it = ginfo.cat.begin(); it != ginfo.cat.end(); it++)
	{
		json_t *one = json_new_number(intToString(*it).c_str());
		json_insert_child(cat, one);
	}

	jsonInsertInt(group, "advcat", ginfo.advcat);
	jsonInsertString(group, "adomain", ginfo.adomain.c_str());

	jsonInsertInt(group, "at", ginfo.at);
	jsonInsertString(group, "dealid", ginfo.dealid.c_str());

	jsonInsertInt(group, "redirect", ginfo.redirect);
	jsonInsertInt(group, "effectmonitor", ginfo.effectmonitor);

	json_t *ext = json_new_object();
	json_insert_pair_into_object(group, "ext", ext);
	jsonInsertString(ext, "advid", ginfo.ext.advid.c_str());

	// 创意
	map<string, vector<string> >::const_iterator it_maps = redis_cashe->groupid.find(groupid);
	if (it_maps == redis_cashe->groupid.end()){
		return;
	}

	json_t *creatives = json_new_object();
	json_insert_pair_into_object(group, "creatives", creatives);
	const vector<string> &maps = it_maps->second;
	for (size_t i = 0; i < maps.size(); i++)
	{
		json_t *creative = json_new_object();
		json_insert_pair_into_object(creatives, maps[i].c_str(), creative);
		add_creative_info(redis_cashe, maps[i], creative);
	}
}

void dump_bid_content(REDIS_INFO *redis_cashe, set<string> &finished, string &data)
{
	json_t *root;
	char *text = NULL;
	root = json_new_object();
	if (root == NULL)
		return;

	jsonInsertString(root, "errorcode", hexToString(redis_cashe->errorcode).c_str());
	jsonInsertInt(root, "timestamp", time(0));
	jsonInsertInt(root, "adx", redis_cashe->adxtemplate.adx);

	json_t *groups = json_new_object();
	json_insert_pair_into_object(root, "groups", groups);

	// 黑白名单项目
	const vector<pair<string, GROUPINFO> > &tempw = redis_cashe->wgroupinfo;
	for (size_t index = 0; index < tempw.size(); index++)
	{
		string groupid = tempw[index].first;
		json_t *group = json_new_object();
		json_insert_pair_into_object(groups, groupid.c_str(), group);
		add_grp_info(redis_cashe, groupid, group, tempw[index].second);
		bool is_finished = finished.find(groupid) != finished.end();
		jsonInsertBool(group, "finished", is_finished);
	}

	// 通投
	const vector<pair<string, GROUPINFO> > &temp = redis_cashe->groupinfo;
	for (size_t index = 0; index < temp.size(); index++)
	{
		string groupid = temp[index].first;
		json_t *group = json_new_object();
		json_insert_pair_into_object(groups, groupid.c_str(), group);
		add_grp_info(redis_cashe, groupid, group, temp[index].second);
		bool is_finished = finished.find(groupid) != finished.end();
		jsonInsertBool(group, "finished", is_finished);
	}

	if (root)
	{
		json_tree_to_string(root, &text);
		data = text;
		free(text);
		text = NULL;
		json_free_value(&root);
	}
}
