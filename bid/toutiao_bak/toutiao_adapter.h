#ifndef TOUTIAO_ADAPTER_H
#define TOUTIAO_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <hiredis/hiredis.h>
#include "../../common/util.h"
#include "../../common/type.h"
#include <stdint.h>
#include "../../common/bid_filter.h"

#define E_DEVICEID_NOT_EXIST     0xE0080000
#define E_DEVICE_NOT_EXIST       0xE0080001
#define E_IMP_SIZE_ZERO          0xE0080002

// 输入adx request
// 函数中返回竞价结果
void toutiao_adapter(IN uint64_t ctx,IN uint8_t index,IN DATATRANSFER *datatransfer,IN RECVDATA *recvdata,OUT string &data);

#endif



