/*
 * pxene_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <algorithm>

#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "pxene_response.h"

extern uint64_t g_logid, g_logid_local;

static bool parsePxeneRequest(IN RECVDATA *recvdata, IN ADXTEMPLATE adxtemp, OUT COM_REQUEST &crequest)
{
	json_t *root = NULL;
	json_t *label = NULL;

	if(recvdata->length == 0)
		return false;

	json_parse_document(&root, recvdata->data);
	if(root == NULL)
		return false;

	if((label = json_find_first_label(root, "id")) != NULL)
		crequest.id = label->child->text;

	if((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;

		if(imp_value != NULL)
		{
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);

			if((label = json_find_first_label(imp_value, "id")) != NULL)
				imp.id = label->child->text;
	
			if((label = json_find_first_label(imp_value, "banner")) != NULL)
			{
				imp.imptype = IMP_BANNER;

				json_t *banner_value = label->child;

				if(banner_value != NULL)
				{
					if((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.w = atoi(label->child->text);
						
					if((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "btype")) != NULL)
					{
						json_t *temp = label->child->child;

						while (temp != NULL)
						{
							imp.banner.btype.insert(atoi(temp->text));
							temp = temp->next;
						}
					}

					if ((label = json_find_first_label(banner_value, "battr")) != NULL)
					{
						json_t *temp = label->child->child;

						while (temp != NULL)
						{
							imp.banner.btype.insert(atoi(temp->text));
							temp = temp->next;
						}
					}
				}		
			}
			else
			{
				//当前仅支持banner
			}

			if((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
			{
				if (adxtemp.ratio != 0)
				{
					imp.bidfloor = atof(label->child->text) / adxtemp.ratio;
				}
			}				

			crequest.imp.push_back(imp);
		}
	}

	if((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if((label = json_find_first_label(app_child, "id")) != NULL)
			crequest.app.id = label->child->text;

		if((label = json_find_first_label(app_child, "name")) != NULL)
			crequest.app.name = label->child->text;

		if ((label = json_find_first_label(app_child, "cat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				crequest.app.cat.insert(atoi(temp->text));
				temp = temp->next;
			}
		}

		if((label = json_find_first_label(app_child, "bundle")) != NULL)
			crequest.app.bundle = label->child->text;
	}

	if((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;

		if((label = json_find_first_label(device_child, "did")) != NULL)
			crequest.device.did = label->child->text;

		if((label = json_find_first_label(device_child, "didtype")) != NULL && label->child->type == JSON_NUMBER)
			crequest.device.didtype = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "dpid")) != NULL)
			crequest.device.dpid = label->child->text;

		if((label = json_find_first_label(device_child, "dpidtype")) != NULL && label->child->type == JSON_NUMBER)
			crequest.device.dpidtype = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "ua")) != NULL)
			crequest.device.ua = label->child->text;

		if((label = json_find_first_label(device_child, "ip")) != NULL)
			crequest.device.ip = label->child->text;

		if((label = json_find_first_label(device_child, "geo")) != NULL)
		{
			json_t *geo_value = label->child;

			if((label = json_find_first_label(geo_value, "lat")) != NULL && label->child->type == JSON_NUMBER)
				crequest.device.geo.lat = atof(label->child->text);// atof to float???

			if((label = json_find_first_label(geo_value, "lon")) != NULL && label->child->type == JSON_NUMBER)
				crequest.device.geo.lat = atof(label->child->text);
		}

		if((label = json_find_first_label(device_child, "carrier")) != NULL && label->child->type == JSON_NUMBER)
			crequest.device.carrier = atof(label->child->text);

		if((label = json_find_first_label(device_child, "make")) != NULL)
			crequest.device.make = label->child->text;

		if((label = json_find_first_label(device_child, "model")) != NULL)
			crequest.device.model = label->child->text;

		if((label = json_find_first_label(device_child, "os")) != NULL && label->child->type == JSON_NUMBER)
			crequest.device.os = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "osv")) != NULL)
			crequest.device.osv = label->child->text;

		if((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
			crequest.device.connectiontype = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
			crequest.device.devicetype = atoi(label->child->text);
	}

	if ((label = json_find_first_label(root, "cur")) != NULL)
	{
		crequest.cur = "CNY";
	}

	if ((label = json_find_first_label(root, "bcat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			crequest.bcat.insert(atoi(temp->text));
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			crequest.badv.insert(temp->text);
			temp = temp->next;
		}
	}

	json_free_value(&root);

	return true;
}

static bool setPxeneJsonResponse(MESSAGERESPONSE &mresponse, string &response_out)
{
	char *text;
	json_t *root, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;

	root = json_new_object();

	jsonInsertString(root, "id", mresponse.id.c_str());

	if(!mresponse.seatbid.empty())
	{
		array_seatbid =  json_new_array();

		for(int i = 0; i < mresponse.seatbid.size(); i++)
		{
			SEATBIDOBJECT &seatbid = mresponse.seatbid[i];

			entry_seatbid = json_new_object();

			if(!seatbid.bid.empty() && seatbid.bid.size() > 0)
			{
				array_bid = json_new_array();
				
				for(int j = 0; j < seatbid.bid.size(); j++)
				{
					COM_BIDOBJECT &bid = seatbid.bid[j].c_bid;

					entry_bid = json_new_object();
		
					jsonInsertString(entry_bid, "impid", bid.impid.c_str());
					jsonInsertFloat(entry_bid, "price", bid.price);
					jsonInsertString(entry_bid, "adid", bid.adid.c_str());

					if (!bid.cat.empty() && bid.cat.size() > 0)
					{
						json_t* array_cat = json_new_array();
						set<uint16_t>:: iterator it = bid.cat.begin();
						for (; it != bid.cat.end(); it++)
						{
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							json_t *tmp = json_new_number(buf);
							json_insert_child(array_cat, tmp);
						}

						json_insert_pair_into_object(entry_bid, "cat", array_cat);
					}

					jsonInsertInt(entry_bid, "type", bid.type);
					jsonInsertInt(entry_bid, "ftype", bid.ftype);
					jsonInsertString(entry_bid, "cbundle", bid.cbundle.c_str());
					jsonInsertString(entry_bid, "adomain", bid.adomain.c_str());
					jsonInsertInt(entry_bid, "w", bid.w);
					jsonInsertInt(entry_bid, "h", bid.h);
					jsonInsertString(entry_bid, "curl", bid.curl.c_str());
					jsonInsertString(entry_bid, "monitorcode", bid.monitorcode.c_str());

					if (!bid.cmonitorurl.empty() && bid.cmonitorurl.size() > 0)
					{
						json_t *array_cmonitorurl = json_new_array();

						for (int k = 0; k < bid.cmonitorurl.size(); k++)
						{
							json_t *tmp = json_new_string(bid.cmonitorurl[k].c_str());
							json_insert_child(array_cmonitorurl, tmp);
						}
	
						json_insert_pair_into_object(entry_bid, "cmonitorurl", array_cmonitorurl);
					}

					if (!bid.imonitorurl.empty() && bid.imonitorurl.size() > 0)
					{
						json_t*array_imonitorurl = json_new_array();

						for (int k = 0; k < bid.imonitorurl.size(); k++)
						{
							json_t *tmp = json_new_string(bid.imonitorurl[k].c_str());
							json_insert_child(array_imonitorurl, tmp);
						}

						json_insert_pair_into_object(entry_bid, "imonitorurl", array_imonitorurl);
					}
					
					jsonInsertString(entry_bid, "sourceurl", bid.sourceurl.c_str());
					
					jsonInsertString(entry_bid, "cid", bid.cid.c_str());

					jsonInsertString(entry_bid, "crid", bid.crid.c_str());

					if (!bid.attr.empty() && bid.attr.size() > 0)
					{
						json_t*array_attr = json_new_array();
						set<uint8_t>::iterator it = bid.attr.begin();
						for (; it != bid.attr.end(); it++)
						{
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							json_t *tmp = json_new_number(buf);
							json_insert_child(array_attr, tmp);
						}

						json_insert_pair_into_object(entry_bid, "attr", array_attr);
					}

					jsonInsertString(entry_bid, "nurl", seatbid.bid[j].nurl.c_str());
					jsonInsertString(entry_bid, "adm", seatbid.bid[j].adm.c_str());
					jsonInsertString(entry_bid, "iurl", seatbid.bid[j].iurl.c_str());

					json_insert_child(array_bid, entry_bid);					
				}

				json_insert_pair_into_object(entry_seatbid, "bid", array_bid);	
			}

			json_insert_child(array_seatbid, entry_seatbid);
		}

		json_insert_pair_into_object(root, "seatbid", array_seatbid);
	}
	
	jsonInsertString(root, "bidid", mresponse.bidid.c_str());

	jsonInsertString(root, "cur", mresponse.cur.c_str());

	json_tree_to_string(root, &text);

	response_out = text;
	
	free(text);
	
	json_free_value(&root);

}

void replace_url(INOUT string &url, IN COM_REQUEST crequest, IN string mapid)
{
	replaceMacro(url, "#BID#", crequest.id.c_str());
	replaceMacro(url, "#MAPID#", mapid.c_str());
	replaceMacro(url, "#DEVICEID#", crequest.device.dpid.c_str());
	replaceMacro(url, "#DEVICEIDTYPE#", intToString(crequest.device.dpidtype).c_str());
}

void transform_response(INOUT COM_RESPONSE &c, IN ADXTEMPLATE adxtemp, IN COM_REQUEST crequest, OUT MESSAGERESPONSE &m)
{
	m.id = c.id;	
	for (int i = 0; i < c.seatbid.size(); i++)
	{
		SEATBIDOBJECT seatbid;
		COM_SEATBIDOBJECT &c_seatbid = c.seatbid[i];

		for (int j = 0; j < c_seatbid.bid.size(); j++)
		{
			int k;
			BIDOBJECT bid;
			COM_BIDOBJECT &c_bid = c_seatbid.bid[j];

			bid.c_bid = c_bid;//复制公共部分
			bid.c_bid.price *= adxtemp.ratio;

			if ((bid.nurl = adxtemp.nurl) != "")
			{
				replace_url(bid.nurl, crequest, c_bid.adid);
			}

			for (k = 0; k < adxtemp.adms.size(); k++)
			{
				if ((adxtemp.adms[k].os == crequest.device.os)&&
					(adxtemp.adms[k].type == c_bid.type))
				{
					string cturl, iurl; 

					cturl = adxtemp.cturl[0];
					replace_url(cturl, crequest, c_bid.adid);

					iurl = adxtemp.iurl[0];
					replace_url(iurl, crequest, c_bid.adid);

					bid.adm = adxtemp.adms[k].adm;
					replaceMacro(bid.adm, "#CTURL#", cturl.c_str());
					replaceMacro(bid.adm, "#IURL#", iurl.c_str());
					replaceMacro(bid.adm, "#SOURCEURL#", c_bid.sourceurl.c_str());

					break;
				}
			}

			if ((bid.iurl = adxtemp.iurl[0]) != "")
			{
				replace_url(bid.iurl, crequest, c_bid.adid);
			}	

			seatbid.bid.push_back(bid);
		}
		m.seatbid.push_back(seatbid);
	}
	m.bidid = c.bidid;
	m.cur = "USD";
}

static void init_message_response(INOUT MESSAGERESPONSE &response)
{
	response.id = "";
	response.cur = "USD";
}

bool pxeneResponse(IN uint64_t ctx, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata)
{
	bool is_data_valid = true;

	int errcode = -1;
	COM_REQUEST crequest;//common msg request
	COM_RESPONSE cresponse;
	MESSAGERESPONSE mresponse;
	ADXTEMPLATE adxtemp;
	string output = "";

	if ((recvdata == NULL) || (ctx == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata or ctx is null");
		is_data_valid = false;
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata->data is null or recvdata->length is 0");
		is_data_valid = false;
		goto exit;
	}

	init_com_template_object(&adxtemp);
	errcode = bid_filter_get_template(ctx, ADX_INMOBI, &adxtemp);
	if (errcode != 0)
	{
		cflog(g_logid_local, LOGERROR, "bid_filter_get_template failed!");
		goto exit;
	}

	init_com_message_request(&crequest);
	if (!parsePxeneRequest(recvdata, adxtemp, crequest))
	{
		cflog(g_logid_local, LOGERROR, "parsePxeneRequest failed!");
		is_data_valid = false;
		goto exit;
	}

	cflog(g_logid_local, LOGINFO, "crequest.imp[0].bidfloor: %f", crequest.imp[0].bidfloor);
//	sendLog(datatransfer, crequest);

	dump_set_cat(g_logid_local, crequest.bcat);

	init_com_message_response(&cresponse);
	errcode = bid_filter_response(ctx, crequest, &cresponse);

exit:

	if (is_data_valid)
	{
		writeLog(g_logid, LOGINFO, string(recvdata->data));
	}

	if (errcode == 0)
	{
		cflog(g_logid_local, LOGINFO, "%s:: cresponse.seatbids.size:%d", __FUNCTION__, cresponse.seatbid.size());

		init_message_response(mresponse);
		transform_response(cresponse, adxtemp, crequest, mresponse);

		cflog(g_logid_local, LOGINFO, "%s:: bid_filter_response:: mresponse.seatbids.size:%d", __FUNCTION__, cresponse.seatbid.size());
		setPxeneJsonResponse(mresponse, output);

		senddata = "Content-Type: application/json\r\nContent-Length: "
			+ intToString(output.size()) + "\r\n\r\n" + output;

		cflog(g_logid_local, LOGINFO, "at fn:%s, ln:%d.", __FUNCTION__, __LINE__);
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	return is_data_valid;
}




