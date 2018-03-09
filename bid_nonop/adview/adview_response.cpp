/*
 * adview_response.cpp
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
#include "adview_response.h"
#include "../../common/errorcode.h"

#define ADS_AMOUNT 20

extern bool fullreqrecord;
extern uint64_t g_logid, g_logid_local;

static int parseadviewRequest(RECVDATA *recvdata, string &requestid)
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;
#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "Begin parseadviewRequest");
#endif
	if (recvdata->length == 0)
		return E_BAD_REQUEST;

	json_parse_document(&root, recvdata->data);

	if (root == NULL)
	{
		writeLog(g_logid_local, LOGINFO, "parseadviewRequest root is NULL!");
		return E_BAD_REQUEST;
	}

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "Parse adview Request success!");
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
	writeLog(g_logid_local, LOGINFO, "End parseadviewRequest");
#endif
exit:
	json_free_value(&root);
	return err;
}

static bool setadviewJsonResponse(string requsetid, string &data_out)
{
	char *text;
	json_t *root, *label_seatbid, *label_bid, *label_ext,
		*array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;

#ifndef DEBUG
	writeLog(g_logid_local, LOGINFO, "Begin setadviewJsonResponse");
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
	writeLog(g_logid_local, LOGINFO, "End setadviewJsonResponse");
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

	ret = parseadviewRequest(recvdata, requestid);
	if(ret == E_BAD_REQUEST)
		goto exit;

	setadviewJsonResponse(requestid, output);
	
	senddata = "Content-Type: application/json\r\nx-adviewrtb-version: 2.3\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;

	
	if(fullreqrecord)
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));

exit:
	return err;
}




