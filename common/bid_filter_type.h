#ifndef _BID_FILTER_TYPE_H_
#define _BID_FILTER_TYPE_H_

#include "type.h"
#include <string.h>

struct com_geo_object
{
	double lat;
	double lon;
};
typedef struct com_geo_object COM_GEOOBJECT;

struct com_device_object
{
	string deviceid;//filled by backend
	uint8_t deviceidtype;//filled by backend
	map<uint8_t, string> dids;//new version
	map<uint8_t, string> dpids;//new version
	string ua;
	string ip;
	int location;
	char locid[DEFAULTLENGTH + 1];
	COM_GEOOBJECT geo;
	uint8_t carrier;
	uint16_t make;//support make target
	string makestr;
	string model;
	uint8_t os;
	string osv;
	uint8_t connectiontype;
	uint8_t devicetype;
};
typedef struct com_device_object COM_DEVICEOBJECT;

struct com_app_object
{
	string id;
	string name;
	set<uint32_t> cat;
	string bundle;
	string storeurl;//download url
};
typedef struct com_app_object COM_APPOBJECT;

struct com_user_object
{
	string id;
	uint8_t gender;
	int yob;
	string keywords;
	string searchkey;
	COM_GEOOBJECT geo;
};
typedef struct com_user_object COM_USEROBJECT;

struct com_banner_object
{
	uint16_t w;
	uint16_t h;
	set<uint8_t> btype;
	set<uint8_t> battr;
};
typedef struct com_banner_object COM_BANNEROBJECT;

struct com_video_object
{
	set<uint8_t> mimes;
	uint8_t display;
	uint32_t minduration;
	uint32_t maxduration;
	uint16_t w;
	uint16_t h;
	set<uint8_t> battr;
	bool is_limit_size;
};
typedef struct com_video_object COM_VIDEOOBJECT;

struct com_title_request_object
{
	uint32_t len;
};
typedef struct com_title_request_object COM_TITLE_REQUEST_OBJECT;

struct com_title_response_object
{
	string text;
};
typedef struct com_title_response_object COM_TITLE_RESPONSE_OBJECT;

struct com_image_request_object
{
	uint8_t type;//AssetImageType: ASSET_IMAGETYPE_ICON or ASSET_IMAGETYPE_MAIN
	uint16_t w;
	uint16_t wmin;
	uint16_t h;
	uint16_t hmin;
	set<uint8_t> mimes;//暂时忽略此参数，调试时注意。另tanx不支持gif
};
typedef struct com_image_request_object COM_IMAGE_REQUEST_OBJECT;

struct com_image_response_object
{
	uint8_t type;//AssetImageType: ASSET_IMAGETYPE_ICON or ASSET_IMAGETYPE_MAIN
	string url;
	uint16_t w;
	uint16_t h;
};
typedef struct com_image_response_object COM_IMAGE_RESPONSE_OBJECT;

struct com_data_request_object
{
	uint8_t type;//AssetDataType: ASSET_DATATYPE_DESC or ASSET_DATATYPE_RATING
	uint32_t len;
};
typedef struct com_data_request_object COM_DATA_REQUEST_OBJECT;

struct com_data_response_object
{
	uint8_t type;//AssetDataType: ASSET_DATATYPE_DESC or ASSET_DATATYPE_RATING
	string label;
	string value;
};
typedef struct com_data_response_object COM_DATA_RESPONSE_OBJECT;

struct com_asset_request_object
{
	uint64_t id;
	uint8_t required;
	uint8_t type;//NativeAssetType
	COM_TITLE_REQUEST_OBJECT title;
	COM_IMAGE_REQUEST_OBJECT img;
	COM_DATA_REQUEST_OBJECT data;
};
typedef struct com_asset_request_object COM_ASSET_REQUEST_OBJECT;

struct com_asset_response_object
{
	uint64_t id;
	uint8_t required;
	uint8_t type;//NativeAssetType
	COM_TITLE_RESPONSE_OBJECT title;
	COM_IMAGE_RESPONSE_OBJECT img;
	COM_DATA_RESPONSE_OBJECT data;
};
typedef struct com_asset_response_object COM_ASSET_RESPONSE_OBJECT;

struct com_native_request_object
{
	uint8_t layout;//NativeLayoutType: NATIVE_LAYOUTTYPE_NEWSFEED
	uint8_t plcmtcnt;
	set<uint8_t> bctype;
	int img_num;
	vector<COM_ASSET_REQUEST_OBJECT> assets;
};
typedef struct com_native_request_object COM_NATIVE_REQUEST_OBJECT;

struct com_native_response_object
{
	vector<COM_ASSET_RESPONSE_OBJECT> assets;
};
typedef struct com_native_response_object COM_NATIVE_RESPONSE_OBJECT;

struct com_impression_ext
{
	uint8_t instl;//是否全插屏广告 0:不是 1:插屏 2:全屏
	uint8_t splash;//是否开屏广告 0:不是 1:是
	string data;//扩展字段，保存请求中需要的字段
	uint16_t advid;//扩展字段，创意id
	uint16_t adv_priority; // 创意id的优先级
	set<int32> productCat;
	set<int32> restrictedCat;
	set<int32> sensitiveCat;
	set<uint32_t> acceptadtype;
};
typedef struct com_impression_ext COM_IMPEXT;

struct com_impression_object
{
	string id;
	uint8_t type;//1:banner 2:video 3:native
	COM_BANNEROBJECT banner;
	COM_VIDEOOBJECT video;
	COM_NATIVE_REQUEST_OBJECT native;
	uint16_t requestidlife;//0:default
	double bidfloor;
	map<int, double> fprice;
	map<string, double> dealprice;
	string bidfloorcur;
	uint8_t adpos;
	COM_IMPEXT ext;//for log, not bid.
};
typedef struct com_impression_object COM_IMPRESSIONOBJECT;

struct com_request
{
	string id;
	vector<COM_IMPRESSIONOBJECT> imp;
	COM_APPOBJECT app;
	COM_DEVICEOBJECT device;
	COM_USEROBJECT user;
	uint8_t at;
	set<string> cur;//Array of allowed currencies for bids on this bid request \
					using ISO-4217 alphabetic codes.  If only one currency is used by th exchange, \
					this parameter is not required.
	set<uint32_t> bcat;
	set<string> badv;
	set<string> wadv;
	int is_recommend;
	int is_secure;
	int support_deep_link;
};
typedef struct com_request COM_REQUEST;

struct com_bid_object
{
	string mapid;
	string impid;
	double price;
	string adid;
	string groupid;
	string groupid_type;
	set<uint32_t> cat;
	uint8_t type;
	uint16_t ftype;
	uint8_t ctype;
	string bundle;
	string apkname;
	string adomain;
	uint16_t w;
	uint16_t h;
	string curl;
	string landingurl;
	uint8_t redirect;
	uint8_t effectmonitor;
	string monitorcode;
	vector<string> cmonitorurl;
	vector<string> imonitorurl;
	string sourceurl;
	string cid;
	string crid;
	set<uint8_t> attr;
	string dealid;
	COM_NATIVE_RESPONSE_OBJECT native;
	GROUPINFO_EXT groupinfo_ext;
	CREATIVE_EXT creative_ext;
};
typedef struct com_bid_object COM_BIDOBJECT;

struct com_seat_bid_object
{
	vector<COM_BIDOBJECT> bid;
};
typedef struct com_seat_bid_object COM_SEATBIDOBJECT;

struct common_response
{
	string id;
	vector<COM_SEATBIDOBJECT> seatbid;
	string bidid;
	string cur;//Only "CNY" is supported.
};
typedef struct common_response COM_RESPONSE;
/*
void dump_vector_combidobject(IN uint64_t logid, IN vector<COM_BIDOBJECT> v_bid);
void dump_set_regioncode(IN uint64_t logid, IN set<uint32_t> regioncode);
void dump_set_cat(IN uint64_t logid, IN set<uint32_t> s_cat);
void dump_set_connectiontype(IN uint64_t logid, IN set<uint8_t> s_connectiontype);
void dump_set_os(IN uint64_t logid, IN set<uint8_t> s_os);
void dump_set_carrier(IN uint64_t logid, IN set<uint8_t> s_carrier);
void dump_set_devicetype(IN uint64_t logid, IN set<uint8_t> s_devicetype);
void dump_vector_mapid(IN uint64_t logid, IN vector<string> v_mapid);
void dump_map_mapinfo(IN uint64_t logid, IN map<string, CREATIVEOBJECT> m_creativeinfo);
void dump_vector_groupid(IN uint64_t logid, IN vector<string> v_groupid);
void dump_map_groupinfo(IN uint64_t logid, IN map<string, GROUPINFO> m_groupinfo);
void dump_map_grouprc(IN uint64_t logid, IN map<string, FREQUENCYCAPPINGRESTRICTION> m_fc);
*/
/* response init function */
void init_com_asset_response_object(COM_ASSET_RESPONSE_OBJECT &asset);
void init_com_native_response_object(COM_NATIVE_RESPONSE_OBJECT &native);
void init_com_bid_object(COM_BIDOBJECT *pcbid);
void init_com_message_response(COM_RESPONSE *pcresponse);

/* request init function */
void init_com_asset_request_object(COM_ASSET_REQUEST_OBJECT &asset);
void init_com_native_request_object(COM_NATIVE_REQUEST_OBJECT &native);
void init_com_banner_request_object(COM_BANNEROBJECT &banner);
void init_com_impression_object(COM_IMPRESSIONOBJECT *pcimp);
void init_com_message_request(COM_REQUEST *pcrequest);

void init_com_template_object(ADXTEMPLATE *padxtemp);

void print_com_request(IN COM_REQUEST &commrequest);
void print_com_response(IN COM_RESPONSE &commresponse);

#endif
