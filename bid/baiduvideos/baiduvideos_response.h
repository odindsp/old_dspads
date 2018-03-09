#ifndef PXENE_RESPONSE_H_
#define PXENE_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/bid_filter.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"

//#define SEAT_INMOBI	"fc56e2b620874561960767c39427298c"

enum RTB_PROTO_TYPE
{
	PROTO_TYPE_UNKNOWN = 0,
	PROTO_TYPE_JSON,
	PROTO_TYPE_PROTOBUF
};


int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);

#endif
