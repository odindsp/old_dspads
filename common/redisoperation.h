#ifndef REDISOPERATION_H_
#define REDISOPERATION_H_

#include <pthread.h>
#include <map>
#include <vector>
#include <set>
#include <iterator>
#include <iostream>
#include <hiredis/hiredis.h>
#include "../common/json.h"
#include "../common/json_util.h"
#include "../common/util.h"
#include "../common/selfsemphore.h"

using namespace std;

typedef struct redis_info
{
	map<string, vector<string> > groupid;
	vector<pair<string, GROUPINFO> > groupinfo;
	vector<pair<string, GROUPINFO> > wgroupinfo;
	map<string, CREATIVEOBJECT> creativeinfo;
	map<string, FREQUENCYCAPPINGRESTRICTION> fc;
	map<string, GROUP_TARGET> group_target;
	ADX_TARGET adx_target;
	map<string, WBLIST> wblist;
	map<string, set<string> > wbdetaillist;
	ADXTEMPLATE adxtemplate;
	map<string, uint8_t>img_imptype;
	map<string, GROUP_SMART_BID_INFO> grsmartbidinfo;
	LOG_CONTROL_INFO logctrlinfo;
	int errorcode;
}REDIS_INFO;

void get_semaphore(IN uint64_t ctx, OUT uint64_t *cache, OUT sem_t *semid);
int init_redis_operation(IN uint64_t logid_local, IN uint8_t adx, IN char *slave_ip, IN int slave_port, IN char *pubsub_ip, IN int pubsub_port, IN bool *run_flag, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx);
void uninit_redis_operation(IN uint64_t ctx);
//void parseStruct(uint64_t addr);

#endif
