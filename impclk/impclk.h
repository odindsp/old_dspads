#ifndef DSP_IMPCLK_H_
#define DSP_IMPCLK_H_
#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <semaphore.h>
#include "../common/setlog.h"
#include "../common/tcp.h"
#include "../common/init_context.h"
#include "../common/errorcode.h"
#include "../common/url_endecode.h"
#include "../common/util.h"
#include "../common/type.h"

#define 	IN
#define 	OUT
#define 	INOUT

#define 	MAX_LEN								128
#define		PXENE_DELIVER_TYPE_IMP				1	//imp
#define		PXENE_DELIVER_TYPE_CLICK			2	//clk
#define NULLSTR(p)  ((p)!=NULL?(p):"")

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
int parseRequest(IN FCGX_Request request, IN  uint64_t gctx, IN int index, IN DATATRANSFER *datatransfer, IN DATATRANSFER *datatransfer_s, IN char *requestaddr, IN char *requestparam);
#endif /* DSP_IMPCLK_H_ */
