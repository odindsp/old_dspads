#ifndef PXENE_RESPONSE_H_
#define PXENE_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/setlog.h"
#include "../../common/util.h"

int get_bid_response(IN RECVDATA *recvdata, OUT string &senddata);

#endif
