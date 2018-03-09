#include "momo_adapter.h"
using namespace std;

void momo_adapter(IN RECVDATA *recvdata, OUT string &data)
{

    com::immomo::moaservice::third::rtb::v12::BidRequest bidrequest;
    bool parsesuccess = false;

   
    if( recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
    {
        goto release;
    }

    parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
    if (!parsesuccess)
    {
        va_cout("parse data from array failed!");
        goto release;
    }

	data = "Status: 204 OK\r\n\r\n";

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
    return;
}
