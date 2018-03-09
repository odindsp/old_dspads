#include "tanx_adapter.h"
#include "tanx-bidding.pb.h"
#include "../../common/setlog.h"
#include <string>
using namespace std;

extern uint64_t g_logid_local;
extern bool fullreqrecord;

// convert tanx request to common request
bool parse_common_request(IN Tanx::BidRequest &bidrequest,OUT Tanx::BidResponse &bidresponse)
{ 

  if (bidrequest.has_version())
       bidresponse.set_version(bidrequest.version());
    else
    {
      return false;
    }

    if (bidrequest.has_bid())
    {
       if (bidrequest.bid().length() != 32)
       {
          writeLog(g_logid_local,LOGERROR, "id = %s the length is not equal 32",bidrequest.bid().c_str());   
          return false;  
       }
      
       bidresponse.set_bid(bidrequest.bid());
    }
    else
    {
     return false;
    }
  
  return true;
}

void tanx_adapter(IN RECVDATA *recvdata,OUT string &data)
{
  char *senddata = NULL;
  uint32_t sendlength = 0;
  Tanx::BidRequest bidrequest;
  Tanx::BidResponse bidresponse;
  bool parsesuccess = false;
  
  if( recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
  {
    writeLog(g_logid_local,LOGERROR, "recv data is error!");
    goto release;
  }

 parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
 if (!parsesuccess)
  {
    writeLog(g_logid_local,LOGERROR, "Parse data from array failed!");
    va_cout("parse data from array failed!");
    goto release;
  }
  parsesuccess = false; 
  parsesuccess = parse_common_request(bidrequest,bidresponse);
   
  if (!parsesuccess)
  {
    writeLog(g_logid_local,LOGERROR, "request format is wrong");
    goto release;
  }
  

 returnresponse:
   sendlength = bidresponse.ByteSize();
   senddata = (char *)calloc(1,sizeof(char) * (sendlength + 1));
   if (senddata == NULL)
   {
    va_cout("allocate memory failure!");
    writeLog(g_logid_local,LOGERROR, "allocate memory failure!");
    goto release;
   }
   bidresponse.SerializeToArray(senddata,sendlength);
   bidresponse.clear_ads();
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
