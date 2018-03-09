#ifndef DSP_IMPCLK_H_
#define DSP_IMPCLK_H_
#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <semaphore.h>
#include "../common/setlog.h"
#include "../common/tcp.h"
#include "../common/init_context.h"
#include "../common/errorcode.h"
#include "../common/url_endecode.h"
#include "../common/util.h"
#include "../common/type.h"

#define 	IN
#define 	OUT
#define 	INOUT

#define 	MAX_LEN								128

int parseRequest(IN FCGX_Request request, IN DATATRANSFER *datatransfer, IN char *requestaddr, IN char *requesturi, IN char *requestparam);

#endif /* DSP_IMPCLK_H_ */
