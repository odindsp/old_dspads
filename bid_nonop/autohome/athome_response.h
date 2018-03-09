#ifndef ATHOME_RESPONSE_H_
#define ATHOME_RESPONSE_H_

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


struct data_response
{
	string version;
	string slotid;
	uint32_t creative_type;
};
typedef struct data_response DATA_RESPONSE;

int getBidResponse(IN RECVDATA *recvdata, OUT string &senddata);

#endif /* ATHOME_RESPONSE_H_ */
