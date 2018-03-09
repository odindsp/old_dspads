/*
 * athome_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include "athome_response.h"

#include <string>
#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <algorithm>

// #include "../../common/setlog.h"
// #include "../../common/tcp.h"
#include "../../common/json_util.h"
#include "../../common/redisoperation.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"


extern uint64_t g_logid, g_logid_local;

int parseAthomeRequest(char *data, string &requestid, DATA_RESPONSE &dresponse)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;

	if(data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);

	if (root == NULL)
	{
		//writeLog(g_logid_local, LOGINFO, "parseAthomeRequest root is NULL!");
		return E_BAD_REQUEST;
	}
	
	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		requestid = label->child->text;
	}
	else
	{
		err =  E_BAD_REQUEST;
		goto exit;
	}
	
	if((label = json_find_first_label(root, "version")) != NULL)
	{
		dresponse.version = label->child->text;
	}

exit:
	if(root != NULL)
		json_free_value(&root);
	return err;
}

static void setAthomeJsonResponse(string requsetid, string &data_out, DATA_RESPONSE dresponse)
{
	char *text = NULL;
	json_t *root;

	root = json_new_object();
	jsonInsertString(root, "id", requsetid.c_str());
	jsonInsertString(root, "version", dresponse.version.c_str());
	jsonInsertInt(root, "is_cm", false);

exit:
	json_tree_to_string(root, &text);
	if(text != NULL)
	{
		data_out = text;
		free(text);
		text = NULL;
	}

	if(root != NULL)
		json_free_value(&root);

#ifndef DEBUG
	//writeLog(g_logid_local, LOGINFO, "End setAthomeJsonResponse");
#endif
	return;
}

int getBidResponse(IN RECVDATA *recvdata,  OUT string &senddata)
{
	int err = 0;
	string output = "";
	string requestid = "";
	DATA_RESPONSE dresponse;

	if (recvdata == NULL)
	{
		err = -1;
		goto exit;
	}

	if ((recvdata->data == NULL)||(recvdata->length == 0))
	{
		err = -1;
		goto exit;
	}

	err = parseAthomeRequest(recvdata->data, requestid, dresponse);
	if(err == E_BAD_REQUEST)
	{
		goto exit;
	}
	setAthomeJsonResponse(requestid, output, dresponse);
	
	senddata = "Content-Type: application/json\r\nx-lertb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

    va_cout("send data=%s ",senddata.c_str());

exit:

	return err;
}




