#ifndef amax_RESPONSE_H_
#define amax_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata);
void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_string_r(IN void *data, const string key_start, const string key_end, const string val);
int parseadwalkerRequest(char *data, COM_REQUEST &crequest);
#endif /* amax_RESPONSE_H_ */
