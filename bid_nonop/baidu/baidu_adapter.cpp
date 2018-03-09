#include "baidu_realtime_bidding.pb.h"
#include <string>
#include "baidu_adapter.h"
using namespace std;

// convert toutiao request to common request
bool parse_common_request(IN BidRequest &bidrequest,OUT BidResponse &bidresponse)
{

  if(bidrequest.has_id())
      bidresponse.set_id(bidrequest.id());
    else
    {
      return false;
    }

  return true;
}

void baidu_adapter(IN RECVDATA *recvdata,OUT string &data)
{
  char *senddata = NULL;
  uint32_t sendlength = 0;
  BidRequest bidrequest;
  BidResponse bidresponse;
  bool parsesuccess = false;

  if( recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
  {
    goto release;
  }

 parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
 if (!parsesuccess)
  {
    goto release;
  }
  parsesuccess = false;
  parsesuccess = parse_common_request(bidrequest,bidresponse);

  if (!parsesuccess)
  {
    goto release;
  }

 returnresponse:
   sendlength = bidresponse.ByteSize();
   senddata = (char *)calloc(1,sizeof(char) * (sendlength + 1));
   if (senddata == NULL)
   {
    goto release;
   }
   bidresponse.SerializeToArray(senddata,sendlength);
 
   data = "Content-Length: " + intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

   #ifdef DEBUG
     cout<<"send = "<<data<<endl;
   #endif
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
