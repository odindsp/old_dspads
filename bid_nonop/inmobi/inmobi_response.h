#ifndef INMOBI_RESPONSE_H_
#define INMOBI_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/bid_filter_type.h"

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
	string idfasha1;
	string idfamd5;
};
typedef struct device_ext DEVICEEXT;

struct device_object
{
	uint8_t dnt;
	string ua;
	string ip;
	GEOOBJECT geo;
	string didsha1;
	string didmd5;
	string dpidsha1;//not find in inmobi request.
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
};
typedef struct app_object APPOBJECT;
//app

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

struct impression_ext
{};
typedef struct impression_ext IMPEXT;

struct impression_object
{
	string id;
	BANNEROBJECT banner;
	uint8_t instl;
	string tagid;//not find in inmobi request.
	double bidfloor;//not find in inmobi request.
	string bidfloorcur;
	vector<string> iframebuster;
	IMPEXT ext;//not find in inmobi request.
};
typedef struct impression_object IMPRESSIONOBJECT;
//impression

//inmobi recvdata: {"id":"0589e2c7-db0b-44f3-9c0c-76b9b9b0ce6f","imp":[{"id":"ed31fa41-81a0-4480-b97c-0a56e4458cc5","banner":{"w":320,"h":480,"id":"170c4dca-300a-426d-92f0-ae575961efbb","pos":1},"instl":0,"bidfloorcur":"USD","iframebuster":["None"]}],"app":{"id":"8511e3c5-4513-4d00-a23f-8566e19e84b3"},"device":{"dnt":1,"ua":"Mozilla/5.0 (Linux; U; Android 4.1.2; es-us; GT-I9070 Build/JZO54K) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30","ip":"113.116.162.229","geo":{"lat":37.7359,"lon":-122.4481,"country":"CHN","type":2},"didsha1":"8248a74c6060ff89da9793e3d651dddc36a997f4","didmd5":"ba918af7f4dd2ece9acf13db6785a01e","dpidmd5":"7669e3fe5b659658715d0687abfccdc5","make":"Apple","model":"iPhone 3GS","os":"android","osv":"4.2.1","devicetype":1},"at":2,"tmax":500,"wseat":["test"],"cur":["USD"]}

struct message_request
{
	string id;
	vector<IMPRESSIONOBJECT> imp;
	SITEOBJECT site;//not find in inmobi request.
	APPOBJECT app;//not find in inmobi request.
	DEVICEOBJECT device;
	uint8_t at;
	uint8_t tmax;
	vector<string> wseat;
	uint8_t allimps;//not find in inmobi request.
	vector<string> cur;
	vector<string> bcat;//not find in inmobi request.
	vector<string> badv;//not find in inmobi request.
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

struct bid_object
{
//	CREATIVEPOLICYOBJECT policy;//read from redis
	string id;//the same to mapid of policy
	string impid;
	double price;
	string adid;
	string nurl;
	string adm;
	vector<string> adomain;
	string iurl;
	string cid;
	string crid;
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
	string cur;//Only USD is supported.
};
typedef struct message_response MESSAGERESPONSE;

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata, OUT string &requestid);

#endif /* INMOBI_RESPONSE_H_ */
