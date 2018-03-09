#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>

#include "../../common/errorcode.h"
#include "../../common/json_util.h"
#include "baiduvideos_response.h"

extern bool fullreqrecord;

static int parse_pxene_request(RECVDATA *recvdata, string &requestid)
{
    json_t *root = NULL;
    json_t *label = NULL;
    int errorcode = E_SUCCESS;

	if(recvdata->length == 0)
	{
		return E_BAD_REQUEST;
	}
		
    json_parse_document(&root, recvdata->data);
    if(root == NULL)
    {
        return E_BAD_REQUEST;
    }


    if((label = json_find_first_label(root, "id")) != NULL)
        requestid = label->child->text;
	else
	{
		errorcode =  E_BAD_REQUEST;
		goto exit;
	}

exit:
	json_free_value(&root);
    return errorcode;
}

static bool setJsonResponse(string requestid, string &response_out)
{
    char *text;
    json_t *root, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
    uint32_t i, j;

    root = json_new_object();
    jsonInsertString(root, "id", requestid.c_str());
	jsonInsertInt(root, "nbr", 0);
exit:
    json_tree_to_string(root, &text);
    response_out = text;
    free(text);
    json_free_value(&root);

}



int get_bid_response(IN RECVDATA *recvdata, OUT string &senddata)
{
    int errcode = E_SUCCESS;
    string output = "";
	string requestid = "";
	int ret;

    if (recvdata == NULL)
    {
		errcode = -1;
        goto release;
    }

    if ((recvdata->data == NULL) || (recvdata->length == 0))
    {
        errcode = -1;
        goto release;
    }

	errcode = parse_pxene_request(recvdata, requestid);

	if (errcode != E_SUCCESS )
	{
		goto release;
	}
	
//	setJsonResponse(requestid,output);

//	senddata = "Content-Length: application/json\r" + intToString(output.size()) + "\r\n\r\n" + output;
	senddata = "Status: 204 OK\r\n\r\n";
	
release:
    return errcode;
}




