#include "iqiyi_adapter.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include <map>
#include <vector>
#include <string>
#include <errno.h>
#include <algorithm>
#include <set>
using namespace std;

extern uint64_t g_logid_local, g_logid, g_logid_response;

int iqiyi_adapter(IN RECVDATA *recvdata, OUT string &senddata)
{
    int err = E_SUCCESS;

    if( recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
    {
        writeLog(g_logid_local, LOGERROR, "recv data is error!");
    }

	senddata = "Status: 204 OK\r\n\r\n";
	
    return err;
}
