/*
 * adwlker_response.cpp
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
#include "../../common/setlog.h"
#include "../../common/json_util.h"
#include "adwalker_response.h"
#include "../../common/errorcode.h"

#define ADS_AMOUNT 20

extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local;

static int parseAdwalkerRequest(RECVDATA *recvdata, string &requestid)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;
#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "Begin parseadwalkerRequest");
#endif
	if (recvdata->length == 0)
	{
		err =  E_BAD_REQUEST;
		goto exit;
	}

	json_parse_document(&root, recvdata->data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseadwalkerRequest root is NULL!");
		err =  E_BAD_REQUEST;
		goto exit;
	}

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "Parse Adwalker Request success!");
#endif
	
	if ((label = json_find_first_label(root, "id")) != NULL)
	{
		requestid = label->child->text;
	}
	else
	{
		err =  E_BAD_REQUEST;
		goto exit;
	}

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "End parseAdwalkerRequest");
#endif
exit:
	if(root != NULL)
		json_free_value(&root);
	return err;
}

static bool setAdwalkerJsonResponse(string requsetid, string &data_out)
{
	char *text;
	json_t *root, *label_seatbid, *label_bid, *label_ext,
		*array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "Begin setAdwalkerJsonResponse");
#endif
	root = json_new_object();
	jsonInsertString(root, "id", requsetid.c_str());
	jsonInsertInt(root, "nbr", 0);
	
exit:
	json_tree_to_string(root, &text);
	data_out = text;
	free(text);
	json_free_value(&root);

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "End setAdwalkerJsonResponse");
#endif
}

int getBidResponse(IN RECVDATA *recvdata, OUT string &senddata)
{
	int err = 0;
	string output = "";
	string requestid = "";
	int ret;

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

	ret = parseAdwalkerRequest(recvdata, requestid);
	if(ret == E_BAD_REQUEST)
		goto exit;

	setAdwalkerJsonResponse(requestid, output);
	
	senddata = "Content-Type: application/json\r\nx-Adwalkerrtb-version: 1.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

	
	if(fullreqrecord)
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));

exit:
	return err;
}




