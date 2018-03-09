#ifndef BAIDU_ADAPTER_H
#define BAIDU_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>
#include "../../common/util.h"
#include "../../common/type.h"
#include <stdint.h>
#include "momortb12.pb.h"
#include "../../common/getlocation.h"

#define E_DEVICEID_NOT_EXIST     0xE0080000
#define E_DEVICE_NOT_EXIST       0xE0080001
#define E_IMP_SIZE_ZERO          0xE0080002
#define E_IS_PING                0xE0080004
#define E_NATIVE_NOT_EXIST         0xE0080008
#define E_NATIVE_TYPE_NOT_SUPPORT         0xE0080010

//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;

#define MY_DEBUG

// 输入adx request
// 函数中返回竞价结果
void momo_adapter(IN RECVDATA *recvdata, OUT string &data);
#endif



