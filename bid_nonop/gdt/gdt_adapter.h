#ifndef GDT_ADAPTER_H
#define GDT_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>

#include <stdint.h>
#include "../../common/getlocation.h"
#include "../../common/type.h"
#include "../gdt/gdt_rtb.pb.h"


//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;
#define MY_DEBUG

//gdt_adx request
int parse_common_request(IN gdt::adx::BidRequest &adrequest,OUT gdt::adx::BidResponse &adresponse);
void gdt_adapter(IN RECVDATA *recvdata, OUT string &data);

#endif
