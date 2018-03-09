/*
 * adlefee_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include "../lefee/adlefee_response.h"

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

int parseAdlefeeRequest(char *data, string &requestid)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;

	if(data == NULL)
		return E_BAD_REQUEST;

	json_parse_document(&root, data);

	if (root == NULL)
	{
		//writeLog(g_logid_local, LOGINFO, "parseAdlefeeRequest root is NULL!");
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
	
exit:
	if(root != NULL)
		json_free_value(&root);
	return err;
}

static void setAdlefeeJsonResponse(string requsetid, string &data_out)
{
	char *text = NULL;
	json_t *root;

	root = json_new_object();
	jsonInsertString(root, "id", requsetid.c_str());
	
	jsonInsertInt(root, "nbr", 0);

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
	//writeLog(g_logid_local, LOGINFO, "End setAdlefeeJsonResponse");
#endif
	return;
}

int getBidResponse(IN RECVDATA *recvdata,  OUT string &senddata)
{
	int err = 0;
	string output = "";
	string requestid = "";

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

	err = parseAdlefeeRequest(recvdata->data, requestid);
	if(err == E_BAD_REQUEST)
	{
		goto exit;
	}
	setAdlefeeJsonResponse(requestid, output);
	
	senddata = "Content-Type: application/json\r\nx-lertb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

    va_cout("send data=%s ",senddata.c_str());

exit:

	return err;
}




