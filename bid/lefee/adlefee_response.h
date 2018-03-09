#ifndef adlefee_RESPONSE_H_
#define adlefee_RESPONSE_H_

#include <fcgiapp.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../../common/setlog.h"
#include "../../common/tcp.h"
#include "../../common/util.h"
#include "../../common/getlocation.h"
#include "../../common/bid_flow_detail.h"
#include "../../common/server_log.h"


struct impression_object
{
	//string id;
	uint32_t bidfloor;
	string bidfloorcur;
	//uint16_t w;
	//uint16_t h;
	//uint8_t pos;
	//vector<string> btype;
	//vector<string> battr;
	uint8_t instl;
	uint8_t splash;
};
typedef struct impression_object IMPRESSIONOBJECT;

struct app_object
{
	//string aid;
	//string name;
	//vector<string> cat;
	string ver;
	string bundle;
	string itid;
	uint8_t paid;
	string storeurl;
	string Keywords;
	string Pid;
	string pub;
};
typedef struct app_object APPOBJECT;

struct device_object
{
	//string did;
	//string dpid;
	string deviceid;//replace Macro
	uint8_t deviceidtype;//filled Macro 
	string mac;
	//string ua;
	//string ip;
	//string country;
	string carrier;
	//string language;
	//string make;
	//string model;
	uint8_t os;
	//string osv;
	//uint8_t connectiontype;
	uint8_t devicetype;
	string loc;
	float density;
	uint16_t sw;
	uint16_t sh;
	uint8_t orientation;
};
typedef struct device_object DEVICEOBJECT;

struct message_request
{
	string id;
	vector<IMPRESSIONOBJECT> imps;
	APPOBJECT app;
	DEVICEOBJECT device;
	set<string> bcat;
	//vector<string> badv;
	string bidfloorcur;
	string cur;
	uint8_t instl;
	uint8_t splash;
};

typedef struct message_request MESSAGEREQUEST;

struct bid_object
{
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
};

typedef struct bid_object BIDOBJECT;

struct seat_bid_object
{
	vector<BIDOBJECT> bids;
	string seat;
};
typedef struct seat_bid_object SEATBIDOBJECT;

struct message_response
{
	string id;
	uint8_t nbr;
	vector<SEATBIDOBJECT> seatbids;
};
typedef struct message_response MESSAGERESPONSE;
int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata,
	OUT string &senddata, OUT FLOW_BID_INFO &flow_bid);
void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val);
void callback_insert_pair_string_r(IN void *data, const string key_start, const string key_end, const string val);
int parseAdlefeeRequest(char *data, COM_REQUEST &crequest);
void init_message_request(MESSAGEREQUEST &mrequest);
#endif /* adlefee_RESPONSE_H_ */
