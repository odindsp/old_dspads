#ifndef DSP_TRACKER_H_
#define DSP_TRACKER_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <semaphore.h>
#include <sstream>                      // std::stringstream

#include "../common/setlog.h"           //std::openLog
#include "../common/tcp.h"              //std::htonl64
#include "../common/init_context.h"     //std::IFFREENULL
#include "../common/errorcode.h"        //std::E_TRACK_XXX
#include "../common/url_endecode.h"   
#include "../common/util.h"             //std::DATATRANSFER、getTime、sendLog、connect/disconnect_log_server
#include "../common/type.h"             //std::GLOBAL_PATH、GLOBAL_CONF_FILE
#include "../common/rdkafka_operator.h" //std::KAFKA_CONF、init_topic_struct
#include "../common/confoperation.h"    //std::GetPrivateProfileInt、GetPrivateProfileString

#define	IN
#define	OUT
#define	INOUT

#define	MAX_LEN				128
#define	PXENE_DELIVER_TYPE_IMP		1	//imp
#define	PXENE_DELIVER_TYPE_CLICK	2	//clk

#define	NULLSTR(p)	((p)!=NULL?(p):"")

struct frequency_info
{
	uint8_t adx;
	uint8_t interval;
	string groupid;
	string mtype;
	uint8_t type;
	vector<int> frequency;
};
typedef struct frequency_info GROUP_FREQUENCY_INFO;

int check_group_frequency_capping(redisContext *ctx_r, redisContext *ctx_w, GROUP_FREQUENCY_INFO *fre_info);
void writeUserRestriction_impclk(uint64_t logid_local, USER_CREATIVE_FREQUENCY_COUNT &ur);
int GroupRestrictionCount(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid_local, GROUP_FREQUENCY_INFO *gr_frequency_info);
static bool check_frequency_capping(uint64_t gctx, int index, REDIS_INFO *cache, uint8_t adx, char *mapid, char *deviceid, char *mtype, double price);
int UserRestrictionCount(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid_local, USER_INFO *ur_frequency_info);
int parseRequest(IN FCGX_Request request, IN  uint64_t gctx, IN int index, IN DATATRANSFER *datatransfer_f, IN DATATRANSFER *datatransfer_k, IN char *requestaddr, IN char *requestparam);

//20170708: add for new tracker
string make_error_message(const string& bmsg, const string& emsg, const string& ecode_str);
string price_normalization(const double price, const int adx);
double parse_notice_price(IN const char *requestparam_, IN DATATRANSFER *datatransfer_f_, IN MONITORCONTEXT *pctx_, IN const string& bid_str_, IN const string& impid_str_, IN const string& mapid_str_, IN const int i_adx_, IN const int i_at_, IN const string& mtype_str_, INOUT int& price_errcode_);
int parse_notice_deviceidinfo(IN const char *requestparam_, IN DATATRANSFER *datatransfer_f_, IN const string& bid_str_, IN const string& impid_str_, IN const string& mapid_str_, IN const int i_adx_, IN const int i_at_, IN const string& mtype_str_, INOUT string& deviceid_str_, INOUT string& deviceidtype_str_, INOUT vector<int>& rtb_errcode_list_);
int send_flume_log(IN const string& base_data, IN const string& err_data, IN const int& errcode, IN DATATRANSFER *datatransfer_f_);
int send_flume_log(IN const string& base_data, IN const string& err_data, IN const vector<int>& errcode_list, IN DATATRANSFER *datatransfer_f_);
int send_kafka_log(IN const char *requestaddr_, IN const char *requestparam_, IN const string& win_kafka_base_data_, IN const bool& is_valid_, IN const vector<int>& rtb_errcode_list_, IN DATATRANSFER* datatransfer_k_);
int handing_not_rtb_process(void);
int handing_rtb_winnotice(IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_, IN const string& bid_str_, IN const string& impid_str_, IN const int i_at_, INOUT const bool& is_valid_, INOUT const bool& not_bid_winnotice_, INOUT const bool& repeated_winnotice_, INOUT vector<int>& rtb_errcode_list_);
int handing_rtb_impnotice(IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_, IN const string& bid_str_, IN const string& impid_str_, IN const int i_at_, INOUT const bool& is_valid_, INOUT const bool& not_bid_impnotice_, INOUT const bool& repeated_impnotice_, INOUT vector<int>& rtb_errcode_list_);
int handing_rtb_clknotice(IN FCGX_Request request_, IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_, IN const string& bid_str_, IN const string& impid_str_, IN const int i_at_, INOUT const bool& is_valid_, INOUT const bool& not_bid_clknotice_, INOUT const bool& repeated_clknotice_, INOUT vector<int>& rtb_errcode_list_);
int handing_rtb_process(IN FCGX_Request request_, IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_);
int parseRequest(IN FCGX_Request request, IN  uint64_t gctx, IN int index, IN DATATRANSFER *datatransfer_f, IN DATATRANSFER *datatransfer_k, IN char *requestaddr, IN char *requestparam);
#endif // DSP_TRACKER_H_
