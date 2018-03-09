#ifndef AMAX_RESPONSE_H_
#define AMAX_RESPONSE_H_
#include "../../common/type.h"
#include "../../common/util.h"
#include "../../common/bid_filter_type.h"
#include "../../common/bid_flow_detail.h"

#define DBG_INFO	{cout<<"fn:"<<__FUNCTION__<<", ln:"<<__LINE__<<endl;}

int letvResponse(IN uint8_t index, IN uint64_t ctx, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);
int parseLetvRequest(char *data, COM_REQUEST &request);
void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);

#endif /* AMAX_RESPONSE_H_ */
