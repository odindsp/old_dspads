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
#include "openrtb-v2_6.pb.h"

#define SEAT_GOYOO "5666447380963b060049c000"

int getBidResponse(IN RECVDATA *recvdata, OUT string &senddata);

#endif /* goyoo_RESPONSE_H_ */
