#ifndef _BID_FLOW_DETAIL_H_
#define _BID_FLOW_DETAIL_H_
#include "json.h"
#include "errorcode.h"
#include "bid_filter_type.h"
#include "type.h"


class FLOW_BID_INFO
{
private:
	string req_id;
	uint64 req_time;
	uint8_t adx;
	string appid;
	uint8_t imp_type; //1:banner 2:video 3:native
	int width;
	int height;
	int devicetype;
	int os;
	int location;
	int connectiontype;
	int carrier;
	int errcode;
	map<string, ERRCODE_GROUP> detail_grp;
	string ext;  // 扩展信息，用于存放各个adx自己的特有信息

protected:
	json_t* get_grp_detail()const;
	json_t* get_creative_detail(const map<string, int> &creatives)const;
	void add_errcode(json_t *json, int err)const;

public:
	FLOW_BID_INFO(uint8_t _adx);

	void reset();
	void set_err(int err);
	void set_req(const COM_REQUEST &commrequest);
	void set_bid_flow(const FULL_ERRCODE &detail);
	void set_ext(json_t *ext_info);

	string to_string()const;
};

#endif
