#ifndef AMAX_RESPONSE_H_
#define AMAX_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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
#define NULLSTR(p)  ((p)!=NULL?(p):"")


void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val);
int sohuResponse(IN uint8_t index, IN uint64_t ctx, IN RECVDATA *arg, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);

#endif /* AMAX_RESPONSE_H_ */
