#ifndef BAIDU_ADAPTER_H
#define BAIDU_ADAPTER_H

#include <stdint.h>
#include <fcgiapp.h>
#include <fcgi_config.h>
#include <hiredis/hiredis.h>
#include "../../common/util.h"
#include "../../common/type.h"
#include "../../common/bid_filter.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"
#include "momortb12.pb.h"

//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;

#define MY_DEBUG

// 输入adx request
// 函数中返回竞价结果
int parse_common_request(IN com::immomo::moaservice::third::rtb::v12::BidRequest &bidrequest, OUT COM_REQUEST &commrequest, OUT int &nativetype);
void momo_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid);
uint64_t iab_to_uint64(string cat);
void transfer_adv_string(IN void *data, const string key_start, const string key_end, const string val);
void transfer_adv_hex(IN void *data, const string key_start, const string key_end, const string val);
#endif



