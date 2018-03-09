#include "google_response.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/google/decode.h"
#include "../../common/url_endecode.h"

#include <string>
#include <errno.h>

using namespace std;
extern uint64_t g_logid_local;
int getBidResponse(IN RECVDATA *recvdata, OUT string &data)
{
    char *senddata = NULL;
    uint32_t sendlength = 0;
    int err = E_SUCCESS;
    BidResponse bidresponse;

returnresponse:
	//If you do not wish to bid on an impression, you can set the processing_time_ms field alone, and leave all other fields empty.

	bidresponse.set_processing_time_ms(1000);
    sendlength = bidresponse.ByteSize();
    senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
    if (senddata == NULL)
    {
        va_cout("allocate memory failure!");
        writeLog(g_logid_local,LOGERROR, "allocate memory failure!");
        goto release;
    }
    bidresponse.SerializeToArray(senddata,sendlength);
    bidresponse.clear_ad();
    data = "Content-Type: application/octet-stream\r\nContent-Length: " + intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);
#ifdef DEBUG
    cout<<"send data = "<<data<<endl;
#endif
   
release:
    if (senddata != NULL)
    {
        free(senddata);
        senddata = NULL;
    }
    return err;
}
