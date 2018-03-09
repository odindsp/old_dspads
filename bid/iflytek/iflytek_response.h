#ifndef IFLYTEK_RESPONSE_H_
#define IFLYTEK_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/bid_filter.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"

#define SEAT_INMOBI	"fc56e2b620874561960767c39427298c"

//user
struct user_ext
{
	string agegroup;
};
typedef struct user_ext USEREXT;
struct user_object
{
	string id;
	string gender;
	uint32_t yob;
	string keywords;
	USEREXT ext;
};
typedef struct user_object USEROBJECT;

//device
struct geo_ext
{
};
typedef struct geo_ext GEOEXT;
struct geo_object
{
	float lat;
	float lon;
	string country;
	string city;//not find in inmobi request.
	GEOEXT ext;
};
typedef struct geo_object GEOOBJECT;

struct device_ext
{
	uint16_t sw;
	uint16_t sh;
	uint8_t brk;
};
typedef struct device_ext DEVICEEXT;

struct device_object
{
	string ua;
	string ip;
	GEOOBJECT geo;
	//inmobi的did和dpid一样，使用IDFA或Androidid
	// 	string did;
	string didsha1;
	string didmd5;
	string dpidsha1;
	string dpidmd5;
	string mac;
	string macsha1;
	string macmd5;
	string ifa;
	string carrier;
	string language;
	string make;
	string model;
	string os;
	string osv;
	uint8_t js;
	uint8_t connectiontype;
	uint8_t devicetype;
	DEVICEEXT ext;
};
typedef struct device_object DEVICEOBJECT;
//device

//app
struct app_ext
{
};
typedef struct app_ext APPEXT;
struct app_object
{
	string id;
	string name;
	vector<string> cat;
	string ver;
	string bundle;
	string paid;
	APPEXT ext;
};
typedef struct app_object APPOBJECT;
//app

//native
struct native_reqest_ext
{
};
typedef struct native_reqest_ext NATIVEREQEXT;

//img
struct img_ext
{
};
typedef struct img_ext IMGEXT;
struct img_object
{
	uint16_t w;
	uint16_t h;
	uint16_t wmin;
	uint16_t hmin;
	vector<string> mimes;
	IMGEXT ext;
};
typedef struct img_object IMGOBJECT;

//icon
struct icon_ext
{
};
typedef struct icon_ext ICONEXT;
struct l_icon_object
{
	uint16_t w;
	uint16_t h;
	uint16_t wmin;
	uint16_t hmin;
	vector<string> mimes;
	ICONEXT ext;
};
typedef struct l_icon_object ICONOBJECT;

//desc
struct desc_ext
{
};
typedef struct desc_ext DESCEXT;
struct desc_object
{
	uint32_t len;
	DESCEXT ext;
};
typedef struct desc_object DESCOBJECT;

//title
struct title_ext
{
};
typedef struct title_ext TITLEEXT;
struct title_object
{
	uint32_t len;
	TITLEEXT ext;
};
typedef struct title_object TITLEOBJECT;

struct native_request
{
	TITLEOBJECT title;
	DESCOBJECT desc;
	ICONOBJECT icon;
	IMGOBJECT img;
	vector<int> assettype;
	NATIVEREQEXT ext;
};
typedef struct native_request NATIVEREQUEST;

struct native_ext
{
};
typedef struct native_ext NATIVEEXT;
struct native_object
{
	string request;
	string ver;
	vector<uint32_t> api;
	uint32_t battr;
	NATIVEREQUEST natreq;
	NATIVEEXT ext;
};
typedef struct native_object NATIVEOBJECT;
//

struct video_ext
{
};
typedef struct video_ext VIDEOEXT;
//video
struct video_object
{
	uint16_t protocol;
	uint16_t w;
	uint16_t h;
	uint32_t minduration;
	uint32_t maxduration;
	uint8_t linearity;
	VIDEOEXT ext;
};
typedef struct video_object VIDEOOBJECT;
//video

//impression
struct banner_ext
{
};
typedef struct banner_ext BANNEREXT;

struct banner_object
{
	uint16_t w;
	uint16_t h;
	BANNEREXT ext;//not find in inmobi request.
};
typedef struct banner_object BANNEROBJECT;

struct impression_ext
{};
typedef struct impression_ext IMPEXT;

struct impression_object
{
	string id;
	BANNEROBJECT banner;
	VIDEOOBJECT video;
	NATIVEOBJECT native;
	uint8_t instl;
	vector<uint8_t> lattr;
	double bidfloor;
	IMPEXT ext;
};
typedef struct impression_object IMPRESSIONOBJECT;
//impression

struct request_ext
{};
typedef struct request_ext REQEXT;
struct message_request
{
	string id;
	vector<IMPRESSIONOBJECT> imp;
	APPOBJECT app;//not find in inmobi request.
	DEVICEOBJECT device;
	USEROBJECT user;
	uint8_t at;
	uint8_t tmax;
	vector<string> wseat;
	vector<string> cur;
	vector<string> bcat;//not find in inmobi request.
	REQEXT ext;
	int is_secure;
};
typedef struct message_request MESSAGEREQUEST;

/* iflytek response */
struct bid_ext
{};
typedef struct bid_ext BIDEXT;

struct native_response_ext
{};
typedef struct native_response_ext NATIVERESEXT;

struct native_response_object
{
	string title;
	string desc;
	string icon;
	string img;
	string landing;
	vector<string> imptrackers;
	vector<string> clicktrackers;
	NATIVERESEXT ext;
};
typedef struct native_response_object NATIVERESOBJECT;

struct video_response_ext
{};
typedef struct video_response_ext VIDEORESEXT;

struct video_response_object
{
	string src;
	uint32_t duration;
	string landing;
	vector<string> starttrackers;
	vector<string> completetrackers;
	vector<string> clicktrackers;
	VIDEORESEXT ext;
};
typedef struct video_response_object VIDEORESOBJECT;

struct bid_object
{
	//	CREATIVEPOLICYOBJECT policy;//read from redis
	string id;
	string impid;
	double price;
	string nurl;
	string adm;
	uint8_t lattr;
	//	vector<uint8_t> attr;
	uint16_t w;
	uint16_t h;
	VIDEORESOBJECT video;
	NATIVERESOBJECT native;
	BIDEXT ext;
};
typedef struct bid_object BIDOBJECT;

struct seat_bid_ext
{};
typedef struct seat_bid_ext SEATBIDEXT;

struct seat_bid_object
{
	vector<BIDOBJECT> bid;
	SEATBIDEXT ext;
};
typedef struct seat_bid_object SEATBIDOBJECT;

struct message_response
{
	string id;
	vector<SEATBIDOBJECT> seatbid;
	string bidid;//the same to request id temporarily.
	string cur;//Default CNY.
};
typedef struct message_response MESSAGERESPONSE;
void init_message_request(MESSAGEREQUEST &mrequest);
bool parse_iflytek_request(char *recvdata, MESSAGEREQUEST &request, int &imptype);
void transform_request(IN MESSAGEREQUEST &m, IN int imptype, OUT COM_REQUEST &c);
int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata,
	OUT string &requestid, OUT string &response_short, OUT FLOW_BID_INFO &flow_bid);

//uint64_t iab_to_uint64(string cat);
void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_hex_string(IN void *data, const string key_start, const string key_end, const string val);

#endif /* IFLYTEK_RESPONSE_H_ */
