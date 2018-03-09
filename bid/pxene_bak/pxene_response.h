#ifndef PXENE_RESPONSE_H_
#define PXENE_RESPONSE_H_

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

struct bid_object
{
	COM_BIDOBJECT c_bid;
	string nurl;
	string adm;
	string iurl;
};
typedef struct bid_object BIDOBJECT;

struct seat_bid_object
{
	vector<BIDOBJECT> bid;
};
typedef struct seat_bid_object SEATBIDOBJECT;

struct message_response
{
	string id;
	vector<SEATBIDOBJECT> seatbid;
	string bidid;
	string cur;
};
typedef struct message_response MESSAGERESPONSE;

bool pxeneResponse(IN uint64_t ctx, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata);

#endif /* PXENE_RESPONSE_H_ */
