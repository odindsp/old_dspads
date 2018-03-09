#ifndef GOOGLE_ADAPTER_H
#define GOOGLE_ADAPTER_H

#include <stdint.h>
#include <fcgiapp.h>
#include <fcgi_config.h>
#include <hiredis/hiredis.h>
#include "realtime-bidding.pb.h"
#include "../../common/util.h"
#include "../../common/type.h"
#include "../../common/bid_filter.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"


#define MY_DEBUG


// 输入adx request
// 函数中返回竞价结果
int parse_common_request(IN BidRequest &bidrequest, OUT COM_REQUEST &crequest, OUT uint64_t &billing_id);
int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid);
void callback_insert_pair_int(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_attr_in(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_attr_out(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_geo_ipb(IN void *data, const string key_start, const string key_end, const string val);
#endif
