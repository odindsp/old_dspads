
#ifndef ZPLAY_RESPONSE_H_
#define ZPLAY_RESPONSE_H_

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


#ifndef _DEBUG
#define _DEBUG
#endif

#define DBG_INFO	{cout<<"fn:"<<__FUNCTION__<<", ln:"<<__LINE__<<endl;}

void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val);
bool zplayResponse(IN uint8_t index, IN RECVDATA *arg, OUT string &senddata);

#endif /* AMAX_RESPONSE_H_ */
