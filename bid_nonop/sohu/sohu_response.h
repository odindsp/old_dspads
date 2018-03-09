#ifndef AMAX_RESPONSE_H_
#define AMAX_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/type.h"
#include "../../common/confoperation.h"
#include "../../common/errorcode.h"
#include "../../common/util.h"

#ifndef _DEBUG
#define _DEBUG
#endif

#define DBG_INFO	{cout<<"fn:"<<__FUNCTION__<<", ln:"<<__LINE__<<endl;}

int sohuResponse(IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata);

#endif /* AMAX_RESPONSE_H_ */
