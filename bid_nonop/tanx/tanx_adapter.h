#ifndef TANX_ADAPTER_H
#define TANX_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <hiredis/hiredis.h>
#include "../../common/util.h"
#include "../../common/type.h"
#include <stdint.h>
#include "../../common/bid_filter.h"

//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;

#define MY_DEBUG

// 输入adx request
// 函数中返回竞价结果
void tanx_adapter(IN RECVDATA *recvdata,OUT string &data);

#endif



