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
#include "../../common/init_context.h"
#include "../../common/confoperation.h"
#include "../../common/errorcode.h"
#include "../../common/getlocation.h"

#define DBG_INFO	{cout<<"fn:"<<__FUNCTION__<<", ln:"<<__LINE__<<endl;}

int letvResponse(IN uint8_t index,  IN uint64_t ctx, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata);
int parseLetvRequest(char *data, COM_REQUEST &request);
void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);

#endif /* AMAX_RESPONSE_H_ */
