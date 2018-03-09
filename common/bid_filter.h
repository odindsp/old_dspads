#ifndef _BID_FILTER_H_
#define _BID_FILTER_H_

#include "bid_filter_type.h"

#define FAIL_SHOW cout<<"fail:"<<__FUNCTION__<<",line:"<<dec<<__LINE__<<endl; 

typedef bool (*CALLBACK_PROCESS_CREQUEST)(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate);
typedef bool (*CALLBACK_CHECK_GROUP_EXT)(IN const COM_REQUEST &crequest, IN const GROUPINFO_EXT &ext);
typedef bool (*CALLBACK_CHECK_PRICE)(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat);
typedef bool (*CALLBACK_CHECK_EXT)(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &ext);

FULL_ERRCODE bid_filter_response(IN uint64_t ctx, IN uint8_t index, INOUT COM_REQUEST &crequest, 
	IN CALLBACK_PROCESS_CREQUEST callback_process_crequest, 
	IN CALLBACK_CHECK_GROUP_EXT callback_check_group_ext,
	IN CALLBACK_CHECK_PRICE callback_check_price, 
	IN CALLBACK_CHECK_EXT callback_check_ext,
	OUT COM_RESPONSE *pcresponse,
	OUT ADXTEMPLATE *padxtemplate);

int bid_filter_initialize(IN uint64_t logid, IN uint8_t adx, IN uint8_t thread_count, IN bool *run_flag, IN char *prifile, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx);

int bid_filter_uninitialize(IN uint64_t ctx);
int bid_time_clear(IN uint64_t ctx);

bool find_set_cat(IN uint64_t logid, IN const set<uint32_t> &cat_fix, IN const set<uint32_t> &cat);

void get_server_content(uint64_t ctx, uint8_t index, string &data);
#endif
