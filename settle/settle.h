#ifndef DSP_SETTLE_H_
#define DSP_SETTLE_H_

#include <hiredis/hiredis.h>
#include <pthread.h>
#include "../common/setlog.h"
#include "../common/tcp.h"
#include "../common/util.h"
#include "../common/init_context.h"
#include "../common/errorcode.h"
#include "../common/type.h"

#define NULLSTR(p)  ((p)!=NULL?(p):"")
int parseRequest(uint64_t gctx, uint8_t index, DATATRANSFER *datatransfer, char *requestaddr, char *requestparam);

#endif /* DSP_UTILS_H_ */
