#include "../gdt/gdt_adapter.h"

#include "../../common/json_util.h"
#include "../../common/errorcode.h"
#include <google/protobuf/text_format.h>
#include <syslog.h>
#include <signal.h>
#include <map>
#include <vector>
#include <string>
#include <errno.h>
#include <algorithm>
#include <set>


#include "../gdt/gdt_rtb.pb.h"
using namespace std;

// convert gdt request to common request
int parse_common_request(IN gdt::adx::BidRequest &adrequest,OUT gdt::adx::BidResponse &adresponse)
{

    if (adrequest.has_id())
    {
    	adresponse.set_request_id(adrequest.id());
    }
    else
    {
    	return false;
    }
    return true;
}



void gdt_adapter(IN RECVDATA *recvdata, OUT string &data)
{
    char *senddata = NULL;
    uint32_t sendlength = 0;
    gdt::adx::BidRequest adrequest;
    gdt::adx::BidResponse adresponse;
    bool parsesuccess = false;


    if( recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
    {
        goto release;
    }

    parsesuccess = adrequest.ParseFromArray(recvdata->data, recvdata->length);
    if (!parsesuccess)
    {
        //va_cout("parse data from array failed!");
        goto release;
    }
    parsesuccess = parse_common_request(adrequest,adresponse);

    if (parsesuccess != E_SUCCESS)
    {
        //va_cout("the request is not suit for bidding!");
        goto returnresponse;
    }

    if (!parsesuccess)
    {
        //va_cout("parse data from array failed!");
        goto release;
    }

returnresponse:

    sendlength = adresponse.ByteSize();
    senddata = (char *)calloc(1,sizeof(char) * (sendlength + 1));
    if (senddata == NULL)
    {
        //va_cout("allocate memory failure!");
        goto release;
    }
    adresponse.SerializeToArray(senddata,sendlength);
    data = "Content-Type: application/octet-stream\r\nContent-Length: " + intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

    va_cout("send data=%s ",data.c_str());


release:
    if (recvdata->data != NULL)
    {
        free(recvdata->data);
        recvdata->data = NULL;
    }

    if (recvdata != NULL)
    {
        free(recvdata);
        recvdata = NULL;
    }

    if (senddata != NULL)
    {
        free(senddata);
        senddata = NULL;
    }
    return;
}

