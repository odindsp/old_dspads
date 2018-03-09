#ifndef REDISIMPCLK_H_
#define REDISIMPCLK_H_

#include <pthread.h>
#include <hiredis/hiredis.h>
#include <map>
#include <vector>
#include <set>
#include <iterator>
#include "../common/json.h"
#include "../common/json_util.h"
#include "../common/util.h"
#include <iostream>
#include "../common/selfsemphore.h"
using namespace std;

typedef struct redis_info
{
	map<string, IMPCLKCREATIVEOBJECT> creativeinfo;
	map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK> fc;

}REDIS_INFO;

void get_semaphore(IN uint64_t ctx, OUT uint64_t *cache, OUT sem_t *semid);
int init_redis_operation(IN uint64_t logid_local, IN uint8_t adx, IN char *slave_ip, IN int slave_port, IN char *pubsub_ip, IN int pubsub_port, IN bool *run_flag, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx);
void uninit_redis_operation(IN uint64_t ctx);

#endif
