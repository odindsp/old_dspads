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

int getBidResponse(IN char *requestaddr, IN char *requestparam,  OUT string &senddata);

#endif /* adview_RESPONSE_H_ */
