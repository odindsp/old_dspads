#ifndef TANX_ADAPTER_H
#define TANX_ADAPTER_H

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
#include "tanx-bidding.pb.h"


//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;

#define MY_DEBUG

// 存储请求过来的native信息
typedef struct native_creative
{
	//    set<string> native_template_id;
	map<int, vector<string> > native_template_id;
	vector<int> required_fields;
	vector<int> recommended_fields;
	int title_max_safe_length;
	int ad_words_max_safe_length;
	string image_size;
	int id;
	int creative_count;
}NATIVE_CREATIVE;

// 输入adx request
// 函数中返回竞价结果
int parse_common_request(IN Tanx::BidRequest &bidrequest, OUT COM_REQUEST &commrequest, 
	OUT Tanx::BidResponse &bidresponse, OUT NATIVE_CREATIVE &nativecreative, OUT uint8_t &viewtype);
void tanx_adapter(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid);
uint64_t iab_to_uint64(string cat);
void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val);
void transfer_adv_hex(IN void *data, const string key_start, const string key_end, const string val);
void transfer_adv_string(IN void *data, const string key_start, const string key_end, const string val);
#endif



