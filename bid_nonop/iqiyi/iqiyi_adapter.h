#ifndef IQIYI_ADAPTER_H
#define IQIYI_ADAPTER_H

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"

// 输入adx request
// 函数中返回竞价结果
void callback_insert_pair_int(IN void *data, const string key_start, const string key_end, const string val);
int iqiyi_adapter(IN RECVDATA *recvdata, OUT string &senddata);

#endif



