/*
 * inmobi_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */
#include "sohuRTB.pb.h"
#include "sohu_response.h"
#include <string.h>


static int TranferToCommonRequest(IN sohuadx::Request &sohu_request, OUT sohuadx::Response &sohu_response)
{
	int errorcode = E_SUCCESS;

	if (sohu_request.version())
	{
		sohu_response.set_version(sohu_request.version());
	}
	else
	{
		errorcode = E_BAD_REQUEST;
		goto exit;
	}

	if (sohu_request.bidid() != "")
	{
		sohu_response.set_bidid(sohu_request.bidid());
	}
	else
	{
		errorcode = E_BAD_REQUEST;
		goto exit;
	}
exit:
	return errorcode;
}

int sohuResponse(IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata)
{
	char *outputdata = NULL;
	
	uint16_t err = E_SUCCESS;

	sohuadx::Request sohu_request;
	sohuadx::Response sohu_response;

	bool parsesuccess = false;

	if ((recvdata == NULL))
	{
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		goto exit;
	}

	parsesuccess = sohu_request.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		goto exit;
	}

	err = TranferToCommonRequest(sohu_request, sohu_response);

	if (err == E_SUCCESS)
	{
		uint32_t outputlength = 0;
		outputlength = sohu_response.ByteSize();
		outputdata = (char *)calloc(1, sizeof(char) * (outputlength + 1));
		if (outputdata != NULL)
		{
			sohu_response.SerializeToArray(outputdata, outputlength);
			sohu_response.clear_seatbid();
			senddata = "Content-Length: " + intToString(outputlength) + "\r\n\r\n" + string(outputdata, outputlength);
		}
	}

exit:
	
	if (recvdata->data != NULL)
	{
		free(recvdata->data);
		recvdata->data = NULL;
	}
		
	if (recvdata != NULL)
	{
		free(recvdata);
		recvdata = NULL;
	}

	if (outputdata != NULL)
	{
		free(outputdata);
		outputdata = NULL;
	}
		
	return err;
}
