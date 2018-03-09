/*
* adview_response.cpp
*
*  Created on: March 20, 2015
*      Author: root
*/

#include <string>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "../../common/errorcode.h"

#include "../../common/json_util.h"
#include "homeTV_response.h"

extern uint64_t g_logid_local;

int getBidResponse(IN char *requestaddr, IN char *requestparam,  OUT string &senddata)
{
	int err = E_SUCCESS;
	json_t *root = NULL;
	char *text = NULL;
	string output;

	root = json_new_object();
	jsonInsertInt(root, "code", 204);
	json_tree_to_string(root, &text);
	if(text != NULL)
	{
		output = text;
		free(text);
		text = NULL;
	}

	if(root != NULL)
		json_free_value(&root);
	senddata = "Content-Type: application/json;charset=UTF-8\r\nContent-Length: "
		+ intToString(output.size()) + "\r\n\r\n" + output;
exit:
	cout <<"senddata :" <<senddata <<endl;
	return err;
}
