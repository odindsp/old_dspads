/*
 * goyoo_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <algorithm>

#include "../../common/json_util.h"
#include "goyoo_response.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"

static int parseGoyooRequest(BidRequest &bidrequest, BidResponse &bidresponse)
{
	 int errorcode = E_SUCCESS;
	 if(bidrequest.has_id())
	 {
		 bidresponse.set_id(bidrequest.id());
		 bidresponse.set_bidid(bidrequest.id());
	 }
	 else
	 {
		 errorcode = E_BAD_REQUEST;
		 goto exit;
	 }

exit:
	 return errorcode;
}

static void setGoyooJsonResponse(OUT BidResponse &bidresponse)
{
	bidresponse.set_nbr(UNKNOWN_ERROR);
	return;
}

int getBidResponse(IN RECVDATA *recvdata, OUT string &senddata)
{
	int err = E_SUCCESS;
	uint32_t sendlength = 0;
	struct timespec ts1, ts2;
	long lasttime;
	string str_ferrcode = "";
	BidRequest bidrequest;
	BidResponse bidresponse;
	char *data = NULL;
	int length = 0;

	if (recvdata == NULL)
	{
		err = E_BAD_REQUEST;
		goto exit;
	}

	if ((recvdata->data == NULL)||(recvdata->length == 0))
	{
		err = E_BAD_REQUEST;
		goto exit;
	}
	getTime(&ts1);

	err = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
	if(!err)
	{
		va_cout("parse data from array failed!");
		err = E_BAD_REQUEST;
		goto exit;
	}

	err = parseGoyooRequest(bidrequest, bidresponse);
	if(err == E_BAD_REQUEST)
	{
		va_cout("bad request");
		goto exit;
	}
	
	setGoyooJsonResponse(bidresponse);
	
	length = bidresponse.ByteSize();
	data = (char *)calloc(1, sizeof(char) * (length + 1));
	if (data == NULL)
	{
		va_cout("allocate memory failure!");
		senddata = "Status: 204 OK\r\n\r\n";
		err = E_MALLOC;
		goto exit;
	}

	bidresponse.SerializeToArray(data, length);
	//bidresponse.SerializeToString(&strData);
	bidresponse.Clear();

	senddata = "Content-Type: application/x-protobuf\r\nContent-Length: " + intToString(length) + "\r\n\r\n" + string(data, length);

	err = 0;

exit:
	if (data != NULL)
	{
		free(data);
		data = NULL;
	}
	return err;
}
