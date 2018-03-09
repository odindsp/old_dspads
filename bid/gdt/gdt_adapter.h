#ifndef GDT_ADAPTER_H
#define GDT_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <hiredis/hiredis.h>
#include <stdint.h>
#include "../../common/bid_flow_detail.h"
#include "../../common/bid_filter.h"
#include "../../common/getlocation.h"
#include "../../common/type.h"
#include "../../common/util.h"
#include "../../common/server_log.h"
#include "gdt_rtb.pb.h"


#define NULLSTR(p)  ((p)!=NULL?(p):"")
//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;
#define MY_DEBUG

//gdt_adx request
int parse_common_request(IN gdt::adx::BidRequest &adrequest, OUT COM_REQUEST &commrequest, OUT gdt::adx::BidResponse &adresponse, OUT string &data);
void gdt_adapter(IN uint64_t ctx, IN uint8_t index,
	IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid);
uint64_t iab_to_uint64(string cat);
void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val);
void transfer_adv_hex(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val);
void transfer_advid_string(IN void *data, const string key_start, const string key_end, const string val);
void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi);
void get_native_info(string impInfo, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi);
void get_banner_info(string impInfo, uint32 &w, uint32 &h, uint32 &proi);

#endif
