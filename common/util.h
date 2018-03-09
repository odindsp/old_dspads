#ifndef UTIL_H_
#define UTIL_H_

#include <queue>
#include <hiredis/hiredis.h>
#include "type.h"
#include "bid_filter.h"
#include <openssl/md5.h>
#include "url_endecode.h"
#include "rdkafka_operator.h"  //std::KAFKA_CONF

struct recv_data
{
        char *data;
        uint32_t length;
		uint32_t buffer_length;
};
typedef struct recv_data RECVDATA;

struct data_transfer
{
        string server;
        int port;
        int fd;
        queue<string> data;
        pthread_mutex_t mutex_data;
        uint64_t logid;
        pthread_t threadlog;
        bool *run_flag;
        bool write;
		bool kafka;
	KAFKA_CONF kafka_conf;   //20170704: add
};
typedef struct data_transfer DATATRANSFER;

bool check_string_type(string data, int len, bool bconj, bool bcolon, uint8_t radix);

long int getStartTime(uint8_t period);
uint32_t getCurrentTimeSecond();
void getlocaltime(char *localtm);
void getTime(timespec *ts);
char *getParamvalue(const char *inputparam, const char *paramname);
string uintToString(uint64_t arg);
string intToString(int arg);
string hexToString(int arg);
string doubleToString(double arg);
redisContext *connectRedisServer(char *ip, int port);
int getFrequencyCappingRestriction(redisContext *context, uint8_t adx, uint64_t logid_local, const char *groupid, FREQUENCYCAPPINGRESTRICTION &fcr);
int getFrequencyCappingRestrictionimpclk(redisContext *context, uint64_t logid_local, const char *groupid, FREQUENCYCAPPINGRESTRICTIONIMPCLK &fcr);
int readUserRestriction(redisContext *context, uint64_t logid_local, string deviceid, USER_FREQUENCY_COUNT &user_restriction);
int writeUserRestriction(redisContext *context, uint64_t logid_local, USER_FREQUENCY_COUNT ur);
void *threadSendLog(void *arg);
////20170704: add for kafka
void *threadSendKafkaLog(void *arg);
string BidInfo2String(const COM_REQUEST &commreq, const COM_RESPONSE &cresp, uint8_t adx, int errcode);
int sendLog(DATATRANSFER *datatransfer, string data);
int sendLogToKafka(DATATRANSFER *datatransfer, string data);
int sendLogdirect(DATATRANSFER *datatransfer,  COM_REQUEST &commreq, uint8_t adx, int errcode);
void replaceMacro(string &str, const char *macro, const char *data);
int selectRedisDB(redisContext *context, uint64_t logid_local, int dbid);
int existRedisKey(redisContext *context, uint64_t logid_local, string key);
int existRedisSetKey(redisContext *context, uint64_t logid_local, string key, string member);
int getRedisValue(redisContext *context, uint64_t logid_local, string key, string &value);
int expireRedisValue(redisContext *context, uint64_t logid_local, string key, int time);
int setexRedisValue(redisContext *context, uint64_t logid_local, string key, int time, string value);
int hgetallRedisValue(redisContext *context, uint64_t logid_local, string key, map<string, string> &m_value);
int hgetRedisValue(redisContext *context, uint64_t logid_local, string key, string field, string &value);
int hsetRedisValue(redisContext *context, uint64_t logid_local, string key, string field, string value);
int get_GROUP_TARGET_object(redisContext *context, uint8_t adx, uint64_t logid_local, string groupid, GROUP_TARGET &target);
int getWBList(redisContext *context, uint64_t logid_local, string groupid, WBLIST &wblist);
int getCreativeInfo(redisContext *context, uint8_t adx, uint64_t logid_local, map<string, uint8_t>img_imptype, string creativeid, CREATIVEOBJECT &creative);
int getGroupInfo(redisContext *context, uint8_t adx, uint64_t logid_local, string groupid, GROUPINFO &ginfo);
int get_ADX_TARGET_object(redisContext *context, uint8_t adx, uint64_t logid_local, ADX_TARGET &target);
int getGroupSmartBidInfo(redisContext *context, uint8_t adx, uint64_t logid_local, string groupid, GROUP_SMART_BID_INFO &grsmartbidinfo);
int getADVCAT(redisContext *context, uint64_t logid_local, map<string, double> &advcat);
int getUerCLKADVCATinfo(redisContext *context, uint64_t logid_local, string userid, USER_CLKADVCAT_INFO &userclkadvcat);
//int getLocation(uint64_t logid_local, const char *ip);

void init_GROUP_TARGET_object(GROUP_TARGET &target);
void init_ADX_TARGET_object(ADX_TARGET &target);
void clear_ADX_TARGET_object(ADX_TARGET &target);
void init_CREATIVE_object(CREATIVEOBJECT &creative);
void init_WBLIST_object(WBLIST &wblist);
void init_FC_object(FREQUENCYCAPPINGRESTRICTION &fcr);
void init_FCIMPCLK_object(FREQUENCYCAPPINGRESTRICTIONIMPCLK &fcr);
void init_GROUPINFO_object(GROUPINFO &ginfo);
void init_ADXTEMPLATE(ADXTEMPLATE &adxinfo);
void init_ADMS(ADMS &adms);
void init_GROUP_SMART_BID(GROUP_SMART_BID_INFO &grsmartbidinfo);
void init_LOG_CONTROL_INFO(LOG_CONTROL_INFO &logctrlinfo);
void clear_ADXTEMPLATE(ADXTEMPLATE &adxinfo);
void va_cout(const char *fmt, ...);

void record_cost(uint64_t logid, bool is_print_time, const char *id, const char *stage, timespec ts1);
void trim_left_right(INOUT string &str);

typedef void (*CALLBACK_INSERT_PAIR)(
	IN void *data, //app cat转换表数据插入回调函数指针
	const string key_start, //key区间下限
	const string key_end, //key区间上限
	const string val //value
	);
bool init_category_table_t(
	IN const string file_path, //app cat 转换文件
	IN CALLBACK_INSERT_PAIR cb_insert_pair, //app cat转换表数据插入回调函数指针
	IN void *data,//app cat 转换表数据结构地址
	IN bool left_is_key = true//true: "=" 左侧一列作为key; false: "=" 右侧一列作为key
	);

string md5String(MD5_CTX hash_ctx, string original);
void replaceAdmasterMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url, string extid);
void replaceNielsenMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url);
void replaceMiaozhenMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url);
void replaceGimcMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url);
void callback_insert_pair_make(IN void *data, const string key_start, const string key_end, const string val);

int set_timer(IN int second);         // 0, success
int set_timer_day(IN uint64_t logid); // 0, success
void trimstring(INOUT string &str);

bool connect_log_server(uint64_t logid, const char *ip, uint16_t port, bool *run_flag, DATATRANSFER *datatransfer);
void disconnect_log_server(DATATRANSFER *datatransfer);

////20170704: add for kafka
bool connect_log_server(uint64_t logid, KAFKA_CONF *kafka_conf, bool *run_flag, DATATRANSFER *datatransfer);////
void disconnect_log_server(DATATRANSFER *datatransfer, bool is_kafka);////

void replaceCurlParam(IN string bid, IN COM_DEVICEOBJECT &cdevice, IN string mapid, INOUT string &url);
void replace_icn_url(IN string bid, IN string mapid, IN COM_DEVICEOBJECT &device, IN string appid, INOUT string &url);
void replace_url(IN string bid, IN string mapid, IN string dpid, IN int deviceidtype, INOUT string &url);
void replace_https_url(IN ADXTEMPLATE &adxinfo, IN int is_secure);

string get_trace_url_par(IN const COM_REQUEST &req, IN const COM_RESPONSE &cresp);
#endif	/* UTIL_H_  */
