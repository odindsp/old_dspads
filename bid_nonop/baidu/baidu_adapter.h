#ifndef BAIDU_ADAPTER_H
#define BAIDU_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>
//#include <stdint.h>
#include "../../common/type.h"
#include <string.h>
#include "../../common/util.h"
//#define MY_DEBUG cout<<__FILE__<<"  "<<__LINE__<<endl;

#define MY_DEBUG

// 输入adx request
// 函数中返回竞价结果
void baidu_adapter(IN RECVDATA *recvdata,OUT string &data);

#endif



