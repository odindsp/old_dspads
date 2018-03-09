#ifndef DSP_URLMONITOR_H_
#define DSP_URLMONITOR_H_
#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <semaphore.h>
#include "../common/setlog.h"
#include "../common/tcp.h"
#include "../common/errorcode.h"
#include "../common/url_endecode.h"
#include "../common/util.h"
#include "../common/type.h"
#include "../common/confoperation.h"

#define IN
#define OUT
#define INOUT
#define	FIRST_JUMP 1
#define OUT_JUMP 2
#define TWICE_JUMP 3

#define FREE(p) {if(p) {free(p);p = NULL;}}
#define NULLSTR(p)  ((p)!=NULL?(p):"") 

int parseRequest(IN FCGX_Request request, IN DATATRANSFER *datatransfer, IN char *requestaddr, IN char *requestparam);
#endif