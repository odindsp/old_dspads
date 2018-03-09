#ifndef _BID_FILTER_DUMP_H_
#define _BID_FILTER_DUMP_H_

#include "type.h"
#include "redisoperation.h"

#define ENTER(logid, level, id) {cflog(logid, level, "%s, %s/%d, enter--", id, __FUNCTION__, __LINE__);}
#define LEAVE(logid, level, id) {cflog(logid, level, "%s, %s/%d, --leave", id, __FUNCTION__, __LINE__);}

void dump_vector_combidobject		(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<COM_BIDOBJECT> &v_bid);
void dump_set_regioncode			(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint32_t> &regioncode);
void dump_set_cat					(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint32_t> &s_cat);
void dump_set_adv					(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<string> &s_adv);
void dump_set_cat_adv				(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint32_t> &s_cat, IN const set<string> &s_adv);
void dump_set_connectiontype		(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_connectiontype);
void dump_set_os					(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_os);
void dump_set_carrier				(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_carrier);
void dump_set_devicetype			(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const set<uint8_t> &s_devicetype);
void dump_set_make					(IN uint64_t logid, IN uint8_t level, IN const char *id, IN set<uint16_t> &s_make);
void dump_vector_mapid				(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<string> &v_mapid);
void dump_map_mapinfo				(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const map<string, CREATIVEOBJECT> &m_creativeinfo);
void dump_vector_groupid			(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<string> &v_groupid);
void dump_vector_pair_groupinfo		(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const vector<pair<string, GROUPINFO> > &v_p_groupinfo);
void dump_map_groupinfo				(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const map<string, GROUPINFO> &m_groupinfo);
void dump_map_grouprc				(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const map<string, FREQUENCYCAPPINGRESTRICTION> &m_fc);
void dump_user_restriction			(IN uint64_t logid, IN uint8_t level, IN const char *id, IN const USER_FREQUENCY_COUNT &ur);

string fullerrcode_to_string(IN const FULL_ERRCODE &ferrcode, IN uint8_t level);//level: FULL_ERRCODE_LEVEL

string bid_detail_record(int secends, int interval, const FULL_ERRCODE &ferrcode);

void dump_bid_content(REDIS_INFO *redis_cashe, set<string> &finished, string &data);

#endif
