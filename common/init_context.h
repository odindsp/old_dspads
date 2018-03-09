#ifndef DSP_INIT_H_
#define DSP_INIT_H_

#include <hiredis/hiredis.h>
#include <stdint.h>
#include "redisimpclk.h"
#include "confoperation.h"
#include "type.h"

#define		PIPETIME			1000
#define     PRICECONF			"dsp_price.conf"
#define IFFREENULL(p) {if(p != NULL){free(p); p = NULL;}}

struct adx_content
{
	int	 name;
	void *dl_handle;
};
typedef struct adx_content ADX_CONTENT;

struct get_context_info
{
	//redis

//	redisContext *redis_filter_master;//Filter deviceid and ip
//	redisContext *redis_filter_slave;
//	redisContext *redis_ur_slave;
	redisContext *redis_id_slave;
	redisContext *redis_price_master; //20170708: add for new tracker
};
typedef struct get_context_info GETCONTEXTINFO;

struct adx_price_info
{
	int adx;
	string groupid;
	string mapid;
	double price;
};
typedef struct adx_price_info ADX_PRICE_INFO;

struct user_info
{
	string deviceid;
	string groupid;
	string mapid;
	FREQUENCY_RESTRICTION_USER fru;
};
typedef struct user_info USER_INFO;

struct pipeline_info
{
	uint64_t logid;
	string message;
	string redisip;
	uint16_t redisport;
	vector<string> *v_cmd;
	pthread_mutex_t *v_mutex;
	bool *run_flag;
};
typedef struct pipeline_info PIPELINE_INFO;

struct monitor_context
{
	uint64_t cache;
	uint64_t logid;
	vector<GETCONTEXTINFO> redis_fds;
	vector<ADX_CONTENT> adx_path;
	vector<ADX_PRICE_INFO> v_price_info;
	pthread_mutex_t mutex_price_info;
	string slave_filter_id_ip;
	uint16_t slave_filter_id_port;
	string master_price_ip;   //20170708: add for new tracker
	string master_price_port; //20170708: add for new tracker
//	string slave_ur_ip;
//	uint16_t slave_ur_port;
	string master_filter_ip;
	uint16_t master_filter_port;
	string slave_filter_ip;
	uint16_t slave_filter_port;
	string messagedata;
	bool *run_flag;
};
typedef struct monitor_context MONITORCONTEXT;

void* threadpipeline(void *arg);
void init_pipeinfo(IN uint64_t logid, IN string message, IN string redisip, IN uint16_t redisport, IN vector<string> *v_cmd, IN pthread_mutex_t *v_mutex, IN bool *run_flag, INOUT PIPELINE_INFO *info);
int Decode(ADX_CONTENT &adx_path, char *encode_price, double &price);
int set_average_price(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid, ADX_PRICE_INFO &price_info);

int global_initialize(IN uint64_t logid, IN uint8_t cpu_count, IN bool *run_flag, IN char *privconf, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx);
int global_uninitialize(uint64 fctx);
#endif
