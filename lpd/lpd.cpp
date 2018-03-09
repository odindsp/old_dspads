#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <hiredis/hiredis.h>
#include "lpd.h"

using namespace std;

extern uint64_t g_logid_local, g_logid;
extern uint64_t geodb;

int parseRequest(IN FCGX_Request request, IN DATATRANSFER *datatransfer, IN char *requestaddr, IN char *requestparam)
{
	int errcode = E_SUCCESS;

	if ((datatransfer == NULL) || (requestaddr == NULL) || (requestparam == NULL))
	{
		errcode = E_INVALID_PARAM;
		writeLog(g_logid_local, LOGERROR, "URL request params invalid,err:0x%x", errcode);
		goto exit;
	}
		sendLog(datatransfer, requestparam);
#ifdef DEBUG
		cout<<"send_data: "<<requestparam<<endl;
#endif
		writeLog(g_logid, LOGDEBUG, string(requestparam));
	
exit:

#ifdef DEBUG
	cout << "odin_lpd exit,errcode: " <<hex<<errcode << endl;
#endif
	return errcode;
}
