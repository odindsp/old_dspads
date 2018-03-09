
#ifndef ZPLAY_RESPONSE_H_
#define ZPLAY_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "openrtb.pb.h"
#include "zplay_adx_rtb.pb.h"
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/bid_filter_type.h"
#include "../../common/type.h"
#include "../../common/confoperation.h"
#include "../../common/errorcode.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"


#ifndef _DEBUG
#define _DEBUG
#endif

#define DBG_INFO	{cout<<"fn:"<<__FUNCTION__<<", ln:"<<__LINE__<<endl;}

void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val);
int zplayResponse(IN uint8_t index, IN uint64_t ctx, IN RECVDATA *arg, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);
static bool TranferToCommonRequest(IN com::google::openrtb::BidRequest &zplay_request, OUT COM_REQUEST &crequest, 
	OUT com::google::openrtb::BidResponse &zplay_response);

#endif /* AMAX_RESPONSE_H_ */
