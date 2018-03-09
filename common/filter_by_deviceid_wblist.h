#ifndef		WBLISTDEVICEID_H_
#define		WBLISTDEVICEID_H_
#include "bid_filter_type.h"
#include "redisoperation.h"
#include <hiredis/hiredis.h>


int filter_group_by_wlist(IN uint64_t logid, IN REDIS_INFO *redis_cashe,
						IN redisContext *redis_wblist_detail, IN const COM_REQUEST &crequest,
						IN const vector<pair<string, GROUPINFO> > &v_p_groupinfo_first_temp,
						OUT vector<pair<string, GROUPINFO> > &v_p_groupinfo_first, OUT map<string, bool> &has_blist,
						OUT uint8_t &deviceidtype, OUT string &deviceid,
						INOUT map<string, ERRCODE_GROUP> &m_err_group, IN bool is_print_time);

#endif
