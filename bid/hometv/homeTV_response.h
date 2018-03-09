#ifndef adview_RESPONSE_H_
#define adview_RESPONSE_H_

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

int getBidResponse(IN  uint64_t ctx, IN int index, IN char *requestaddr,
	IN char *requestparam, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);
void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_string_r(IN void *data, const string key_start, const string key_end, const string val);
int parseHomeTVRequest(IN char *requestparam, IN char *reqaddr, OUT COM_REQUEST &crequest);
#endif /* adview_RESPONSE_H_ */
