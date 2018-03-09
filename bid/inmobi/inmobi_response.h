#ifndef INMOBI_RESPONSE_H_
#define INMOBI_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/bid_flow_detail.h"
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/bid_filter.h"
#include "../../common/getlocation.h"
#include "../../common/server_log.h"

#define SEAT_INMOBI	"fc56e2b620874561960767c39427298c"
//common
struct publisher_object
{
	string id;
	string name;
	vector<string> cat;
	string domain;
};
typedef struct publisher_object PUBLISHEROBJECT;

typedef struct publisher_object PRODUCEROBJECT;

struct content_object
{
	string id;
	uint32_t episode;
	string title;
	string series;
	string season;
	string url;
	vector<string> cat;
	uint8_t videoquality;
	string keywords;
	string contentrating;
	string userrating;
	string context;
	uint8_t livestream;
	uint8_t sourcerelationship;
	PRODUCEROBJECT producer;
	uint32_t len;
};
typedef struct content_object CONTENTOBJECT;
//common

//device
struct geo_object
{
	float lat;
	float lon;
	string country;
	string region;//not find in inmobi request.
	string regionfips104;//not find in inmobi request.
	string metro;//not find in inmobi request.
	string city;//not find in inmobi request.
	string zip;//not find in inmobi request.
	uint8_t type;
};
typedef struct geo_object GEOOBJECT;

struct device_ext
{
	string idfa;
	string idfasha1;
	string idfamd5;
	string gpid;
};
typedef struct device_ext DEVICEEXT;

struct device_object
{
	uint8_t dnt;
	string ua;
	string ip;
	GEOOBJECT geo;
	// 	string did;
	string didsha1;
	string didmd5;
	string dpid;
	string dpidsha1;
	string dpidmd5;
	string ipv6;//not find in inmobi request.
	string carrier;//not find in inmobi request.
	string language;//not find in inmobi request.
	string make;
	string model;
	string os;
	string osv;
	uint8_t js;//not find in inmobi request.
	uint8_t connectiontype;//not find in inmobi request.
	uint8_t devicetype;
	string flashver;//not find in inmobi request.
	DEVICEEXT ext;//not find in inmobi request.
};
typedef struct device_object DEVICEOBJECT;
//device

//app
struct app_object
{
	string id;
	string name;
	string domain;
	vector<string> cat;
	vector<string> sectioncat;
	vector<string> pagecat;
	string ver;
	string bundle;
	uint8_t privacypolicy;
	uint8_t paid;
	PUBLISHEROBJECT publisher;
	CONTENTOBJECT content;
	string keywords;
	string storeurl;//download url
};
typedef struct app_object APPOBJECT;
//app

// user
struct user_object
{
	string id;
	int yob;
	string gender;
};
typedef struct user_object USEROBJECT;

//site
struct site_object
{
	string id;
	string name;
	string domain;
	vector<string> cat;
	vector<string> sectioncat;
	vector<string> pagecat;
	string page;
	uint8_t privacypolicy;
	string ref;
	string search;
	PUBLISHEROBJECT publisher;
	CONTENTOBJECT content;
	string keywords;
};
typedef struct site_object SITEOBJECT;
//site

//impression
struct banner_ext
{
	uint8_t adh;
	string orientation;
	uint8_t orientationlock;
};
typedef struct banner_ext BANNEREXT;

struct banner_object
{
	uint16_t w;
	uint16_t h;
	string id;
	uint32_t pos;
	vector<uint8_t> btype;
	vector<uint8_t> battr;
	vector<string> mimes;
	uint32_t topframe;//not find in inmobi request.
	vector<uint8_t> expdir;
	vector<uint8_t> api;
	BANNEREXT ext;//not find in inmobi request.
};
typedef struct banner_object BANNEROBJECT;
struct title_object
{
	uint32_t len;
};
typedef struct title_object TITLEOBJECT;

struct img_object
{
	uint32_t type;
	uint32_t w;
	uint32_t h;
	uint32_t wmin;
	uint32_t hmin;
	vector<string> mimes;
};
typedef struct img_object IMGOBJECT;

struct data_object
{
	uint32_t type;
	uint64_t len;
};
typedef struct data_object DATAOBJECT;

struct asset_object
{
	uint32_t id;
	uint8_t required;
	uint8_t type; // 1,title,2,img,3,data
	TITLEOBJECT title;
	IMGOBJECT img;
	DATAOBJECT data;
};
typedef struct asset_object ASSETOBJECT;

struct native_request
{
	string ver;
	uint8_t layout;
	uint32_t adunit;
	uint32_t plcmtnt;
	uint32_t seq;
	vector<ASSETOBJECT> assets;
};
typedef struct native_request NATIVEREQUEST;

struct native_object
{
	NATIVEREQUEST requestobj;
	string ver;
	vector<uint8_t> api;
};
typedef struct native_object NATIVEOBJECT;

struct deal_object
{
	string id;
	double bidfloor;
	string bidfloorcur;
	uint32_t at;
};
typedef struct deal_object DEALOBJECT;

struct pmp_object
{
	uint32_t private_auction;
	vector<DEALOBJECT> deals;
};
typedef struct pmp_object PMPOBJECT;

struct impression_ext
{};
typedef struct impression_ext IMPEXT;

struct impression_object
{
	string id;
	BANNEROBJECT banner;
	NATIVEOBJECT native;
	PMPOBJECT pmp;
	uint8_t instl;
	string tagid;//not find in inmobi request.
	double bidfloor;//not find in inmobi request.
	string bidfloorcur;
	vector<string> iframebuster;
	IMPEXT ext;//not find in inmobi request.
};
typedef struct impression_object IMPRESSIONOBJECT;
//impression

struct message_request
{
	string id;
	vector<IMPRESSIONOBJECT> imp;
	SITEOBJECT site;//not find in inmobi request.
	APPOBJECT app;//not find in inmobi request.
	DEVICEOBJECT device;
	USEROBJECT user;
	uint8_t at;
	uint8_t tmax;
	vector<string> wseat;
	uint8_t allimps;//not find in inmobi request.
	vector<string> cur;
	vector<string> bcat;//not find in inmobi request.
	vector<string> badv;//not find in inmobi request.
	int is_secure;
};
typedef struct message_request MESSAGEREQUEST;

/* inmobi response */
struct bid_ext
{};
typedef struct bid_ext BIDEXT;

struct creative_policy_object
{
	string mapid;//id
	string groupid;
	double ratio;
	double mprice;
	vector<string> wlid;
	vector<string> blid;
};
typedef struct creative_policy_object CREATIVEPOLICYOBJECT;

struct link_response
{
	string url;
	vector<string> clicktrackers;
	string fallback;
};
typedef struct link_response LINKRESPONSE;

struct data_response
{
	string label;
	string value;
};
typedef struct data_response DATARESPONSE;

struct img_response
{
	string url;
	uint32_t w;
	uint32_t h;
};
typedef struct img_response IMGRESPONSE;

struct title_response
{
	string text;
};
typedef struct title_response TITLERESPONSE;

struct asset_response
{
	uint32_t id;
	uint8_t required;
	uint8_t type;
	TITLERESPONSE title;
	IMGRESPONSE img;
	DATARESPONSE data;
	LINKRESPONSE link;
};
typedef struct asset_response ASSETRESPONSE;

struct native_response
{
	uint32_t ver;
	vector<ASSETRESPONSE> assets;
	LINKRESPONSE link;
	vector<string> imptrackers;
	string jstracker;
};
typedef struct native_response NATIVERESPONSE;

struct adm_object
{
	NATIVERESPONSE native;
};
typedef struct adm_object ADMOBJECT;

struct bid_object
{
	//	CREATIVEPOLICYOBJECT policy;//read from redis
	string id;//the same to mapid of policy
	string impid;
	double price;
	string adid;
	string nurl;
	string adm;
	ADMOBJECT admobject;
	vector<string> adomain;
	string iurl;
	string cid;
	string crid;
	string dealid;
	vector<uint8_t> attr;
	vector<BIDEXT> exts;
};
typedef struct bid_object BIDOBJECT;

struct seat_bid_ext
{};
typedef struct seat_bid_ext SEATBIDEXT;

struct seat_bid_object
{
	vector<BIDOBJECT> bid;
	string seat;//InMobi provided advertiser id: fc56e2b620874561960767c39427298c
	//string group //now don't support
	SEATBIDEXT ext;
};
typedef struct seat_bid_object SEATBIDOBJECT;

struct message_response
{
	string id;
	vector<SEATBIDOBJECT> seatbid;
	string bidid;//the same to request id temporarily.
	string cur;//Default USD.
};
typedef struct message_response MESSAGERESPONSE;

void init_message_request(MESSAGEREQUEST &mrequest);
bool parse_inmobi_request(char *recvdata, MESSAGEREQUEST &request, int &imptype);
int transform_request(IN MESSAGEREQUEST &m, IN int imptype, OUT COM_REQUEST &c);
int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata,
	OUT string &senddata, OUT string &requestid, OUT string &response_short, OUT FLOW_BID_INFO &flow_bid);

//uint64_t iab_to_uint64(string cat);
void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_hex_string(IN void *data, const string key_start, const string key_end, const string val);

#endif /* INMOBI_RESPONSE_H_ */
