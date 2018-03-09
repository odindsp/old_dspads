#ifndef goyoo_RESPONSE_H_
#define goyoo_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"
#include "openrtb-v2_6.pb.h"

#define SEAT_GOYOO "5666447380963b060049c000"

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);
void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_string_r(IN void *data, const string key_start, const string key_end, const string val);
int parseGoyooRequest(BidRequest &bidrequest, COM_REQUEST &crequest, BidResponse &bidresponse);

#endif /* goyoo_RESPONSE_H_ */
