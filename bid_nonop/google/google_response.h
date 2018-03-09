#ifndef GOOGLE_RESPONSE_H
#define GOOGLE_RESPONSE_H

#include <fcgiapp.h>
#include <fcgi_config.h>
#include "../../common/util.h"
#include "../../common/type.h"
#include <stdint.h>
#include "realtime-bidding.pb.h"

int getBidResponse(IN RECVDATA *recvdata, OUT string &data);

#endif



