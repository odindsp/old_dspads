/*
 * inmobi_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <algorithm>

#include "../../common/errorcode.h"
#include "../../common/json_util.h"
#include "inmobi_response.h"

//extern uint64_t g_logid, g_logid_local;

static void init_publisher_object(PUBLISHEROBJECT &publisher)//
{
	publisher.id = publisher.name = publisher.domain = "";
}

static void init_producer_object(PRODUCEROBJECT &producer)//
{
	init_publisher_object((PUBLISHEROBJECT&)producer);
}

static void init_content_object(CONTENTOBJECT &content)//
{
//	content.id = content.title = content.series = content.season = content.url = content.keywords = 
//		content.contentrating = content.userrating = content.context = "";

	content.episode = content.videoquality = content.livestream = content.sourcerelationship = content.len;

	init_producer_object(content.producer);
}

static void init_geo_object(GEOOBJECT &geo)//
{
	geo.lat = geo.lon = geo.type = 0;

//	geo.country = geo.region = geo.regionfips104 = geo.metro = geo.city = geo.zip = "";
}

static void init_site_object(SITEOBJECT &site)//
{
//	site.id = site.name = site.domain = site.ref = site.search = site.keywords = "";

	site.privacypolicy = 0;

	init_publisher_object(site.publisher);

	init_content_object(site.content);
}

static void init_app_object(APPOBJECT &app)//
{
//	app.id = app.name = app.domain = app.ver = app.bundle = app.keywords = "";

	app.privacypolicy = app.paid = 0;

	init_publisher_object(app.publisher);

	init_content_object(app.content);
}

static void init_device_object(DEVICEOBJECT &device)//
{
	device.dnt = device.js = device.connectiontype = device.devicetype = 0;

	init_geo_object(device.geo);

//	device.ua = device.ip = device.didsha1 = device.didmd5 = device.dpidsha1 = device.dpidmd5 = device.ipv6
//		= device.carrier = device.language = device.make = device.model = device.os = device.osv = device.flashver = "";
}

static void init_message_request(MESSAGEREQUEST &mrequest)//
{
//	mrequest.id = "";

	init_site_object(mrequest.site);

	init_app_object(mrequest.app);

	init_device_object(mrequest.device);

	mrequest.at = 2;
	
	mrequest.tmax = mrequest.allimps = 0;
}

static void init_banner_ext(BANNEREXT &ext)
{
	ext.adh = ext.orientationlock = 0;

//	ext.orientation = "";
}

static void init_banner_object(BANNEROBJECT &banner)
{
	banner.w = banner.h = banner.pos = banner.topframe = 0;

//	banner.id = "";

	init_banner_ext(banner.ext);
}

static void init_impression_object(IMPRESSIONOBJECT &imp)
{
//	imp.id = imp.tagid = "";
	
	imp.bidfloorcur = "USD";

	imp.instl = imp.bidfloor = 0;

	init_banner_object(imp.banner);
}

static void init_seat_bid(SEATBIDOBJECT &seatbid)
{
//	seatbid.seat = "";
}

static void init_bid_object(BIDOBJECT &bid)
{
}

static void init_message_response(MESSAGERESPONSE &response)
{
//	response.id = "";
	response.cur = "USD";
}

static bool parseInmobiRequest(RECVDATA *recvdata, MESSAGEREQUEST &request)
{
	json_t *root = NULL;
	json_t *label = NULL;

	if(recvdata->length == 0)
		return false;

	json_parse_document(&root, recvdata->data);
	if(root == NULL)
		return false;

	if((label = json_find_first_label(root, "id")) != NULL)
	{
		request.id = label->child->text;
	}

	if((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;

		if(imp_value != NULL)
		{
			IMPRESSIONOBJECT imp;

			init_impression_object(imp);

			if((label = json_find_first_label(imp_value, "id")) != NULL)
				imp.id = label->child->text;
	
			if((label = json_find_first_label(imp_value, "banner")) != NULL)
			{
				json_t *banner_value = label->child;

				if(banner_value != NULL)
				{
					//BANNEROBJECT bannerobject;

					if(label = json_find_first_label(banner_value, "w"))
						imp.banner.w = atoi(label->child->text);
						
					if((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);

					if((label = json_find_first_label(banner_value, "id")) != NULL)
						imp.banner.id = label->child->text;

					if((label = json_find_first_label(banner_value, "pos")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.pos = atoi(label->child->text);

					if((label = json_find_first_label(banner_value, "topframe")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.topframe = atoi(label->child->text);

					if((label = json_find_first_label(imp_value, "ext")) != NULL)
					{
						json_t *ext_value = label->child;
						if((label = json_find_first_label(ext_value, "adh")) != NULL && label->child->type == JSON_NUMBER)
							imp.banner.ext.adh = atoi(label->child->text);
						if((label = json_find_first_label(ext_value, "orientation")) != NULL)
							imp.banner.ext.orientation = label->child->text;
						if((label = json_find_first_label(ext_value, "orientationlock")) != NULL && label->child->type == JSON_NUMBER)
							imp.banner.ext.orientationlock = atoi(label->child->text);
					}	
				}
			}

			if((label = json_find_first_label(imp_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
				imp.instl = atoi(label->child->text);

			if((label = json_find_first_label(imp_value, "tagid")) != NULL)
				imp.tagid = label->child->text;

			if((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
			{
				imp.bidfloor = atof(label->child->text);
			//	cout<<"imp.bidfloor: "<<imp.bidfloor<<endl;
			}				
			//else
			//	cout<<"not find imp.bidfloor: "<<imp.bidfloor<<" !!!"<<endl;

			if((label = json_find_first_label(imp_value, "bidfloorcur")) != NULL && label->child->type == JSON_NUMBER)
				imp.bidfloorcur = atoi(label->child->text);

			if((label = json_find_first_label(imp_value, "ext")) != NULL)
			{
			}

			request.imp.push_back(imp);
		}
	}

	if((label = json_find_first_label(root, "site")) != NULL)
	{
		json_t *site_child = label->child;

		if((label = json_find_first_label(site_child, "id")) != NULL)
			request.site.id = label->child->text;

		if((label = json_find_first_label(site_child, "name")) != NULL)
			request.site.name = label->child->text;

		if((label = json_find_first_label(site_child, "domain")) != NULL)
			request.site.domain = label->child->text;

		if ((label = json_find_first_label(site_child, "cat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.site.cat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(site_child, "sectioncat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.site.sectioncat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(site_child, "pagecat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.app.pagecat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if((label = json_find_first_label(site_child, "privacypolicy")) != NULL && label->child->type == JSON_NUMBER)
			request.site.privacypolicy = atoi(label->child->text);

		if((label = json_find_first_label(site_child, "ref")) != NULL)
			request.site.ref = label->child->text;

		if((label = json_find_first_label(site_child, "search")) != NULL)
			request.site.search = label->child->text;

		//pub
		if((label = json_find_first_label(site_child, "publisher")) != NULL)
		{
			json_t *publisher_value = label->child;

			if(publisher_value != NULL)
			{
				if((label = json_find_first_label(publisher_value, "id")) != NULL)
					request.site.publisher.id = label->child->text;

				if((label = json_find_first_label(publisher_value, "name")) != NULL)
					request.site.publisher.name = label->child->text;

				if ((label = json_find_first_label(publisher_value, "cat")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						request.site.publisher.cat.push_back(temp->text);
						temp = temp->next;
					}
				}

				if((label = json_find_first_label(site_child, "domain")) != NULL)
					request.site.publisher.domain = label->child->text;
			}
		}

		//content
		if((label = json_find_first_label(site_child, "content")) != NULL)
		{
			json_t *content_value = label->child;

			if(content_value != NULL)
			{
				if((label = json_find_first_label(content_value, "id")) != NULL)
					request.site.content.id = label->child->text;

				if((label = json_find_first_label(content_value, "episode")) != NULL)
					request.site.content.episode = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "title")) != NULL)
					request.site.content.title = label->child->text;

				if((label = json_find_first_label(content_value, "series")) != NULL)
					request.site.content.series = label->child->text;

				if((label = json_find_first_label(content_value, "season")) != NULL)
					request.site.content.season = label->child->text;

				if((label = json_find_first_label(content_value, "url")) != NULL)
					request.site.content.url = label->child->text;

				if ((label = json_find_first_label(content_value, "cat")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						request.site.content.cat.push_back(temp->text);
						temp = temp->next;
					}
				}

				if((label = json_find_first_label(content_value, "videoquality")) != NULL)
					request.site.content.videoquality = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "keywords")) != NULL)
					request.site.content.keywords = label->child->text;

				if((label = json_find_first_label(content_value, "contentrating")) != NULL)
					request.site.content.contentrating = label->child->text;

				if((label = json_find_first_label(content_value, "userrating")) != NULL)
					request.site.content.userrating = label->child->text;

				if((label = json_find_first_label(content_value, "context")) != NULL)
					request.site.content.context = label->child->text;


				if((label = json_find_first_label(content_value, "livestream")) != NULL)
					request.site.content.livestream = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "sourcerelationship")) != NULL)
					request.site.content.sourcerelationship = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "producer")) != NULL)
				{
					json_t *producer_value = label->child;

					if(producer_value != NULL)
					{
						if((label = json_find_first_label(producer_value, "id")) != NULL)
							request.site.content.producer.id = label->child->text;

						if((label = json_find_first_label(producer_value, "name")) != NULL)
							request.site.content.producer.name = label->child->text;

						if ((label = json_find_first_label(producer_value, "cat")) != NULL)
						{
							json_t *temp = label->child->child;

							while (temp != NULL)
							{
								request.site.content.producer.cat.push_back(temp->text);
								temp = temp->next;
							}
						}

						if((label = json_find_first_label(producer_value, "domain")) != NULL)
							request.site.content.producer.domain = label->child->text;
					}
				}

				if((label = json_find_first_label(content_value, "len")) != NULL)
					request.site.content.len = atoi(label->child->text);
			}
		}
		
		if((label = json_find_first_label(site_child, "keywords")) != NULL)
			request.site.keywords = label->child->text;
	}

	if((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if((label = json_find_first_label(app_child, "id")) != NULL)
			request.app.id = label->child->text;

		if((label = json_find_first_label(app_child, "name")) != NULL)
			request.app.name = label->child->text;

		if((label = json_find_first_label(app_child, "domain")) != NULL)
			request.app.domain = label->child->text;

		if ((label = json_find_first_label(app_child, "cat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.app.cat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(app_child, "sectioncat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.app.sectioncat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(app_child, "pagecat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				request.app.pagecat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if((label = json_find_first_label(app_child, "ver")) != NULL)
			request.app.ver = label->child->text;

		if((label = json_find_first_label(app_child, "bundle")) != NULL)
			request.app.bundle = label->child->text;

		if((label = json_find_first_label(app_child, "privacypolicy")) != NULL && label->child->type == JSON_NUMBER)
			request.app.privacypolicy = atoi(label->child->text);

		if((label = json_find_first_label(app_child, "paid")) != NULL && label->child->type == JSON_NUMBER)
			request.app.paid = atoi(label->child->text);

		//pub
		if((label = json_find_first_label(app_child, "publisher")) != NULL)
		{
			json_t *publisher_value = label->child;

			if(publisher_value != NULL)
			{
				if((label = json_find_first_label(publisher_value, "id")) != NULL)
					request.app.publisher.id = label->child->text;

				if((label = json_find_first_label(publisher_value, "name")) != NULL)
					request.app.publisher.name = label->child->text;

				if ((label = json_find_first_label(publisher_value, "cat")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						request.app.publisher.cat.push_back(temp->text);
						temp = temp->next;
					}
				}

				if((label = json_find_first_label(publisher_value, "domain")) != NULL)
					request.app.publisher.domain = label->child->text;
			}
		}

		//content
		if((label = json_find_first_label(app_child, "content")) != NULL)
		{
			json_t *content_value = label->child;

			if(content_value != NULL)
			{
				if((label = json_find_first_label(content_value, "id")) != NULL)
					request.app.content.id = label->child->text;

				if((label = json_find_first_label(content_value, "episode")) != NULL)
					request.app.content.episode = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "title")) != NULL)
					request.app.content.title = label->child->text;

				if((label = json_find_first_label(content_value, "series")) != NULL)
					request.app.content.series = label->child->text;

				if((label = json_find_first_label(content_value, "season")) != NULL)
					request.app.content.season = label->child->text;

				if((label = json_find_first_label(content_value, "url")) != NULL)
					request.app.content.url = label->child->text;

				if ((label = json_find_first_label(content_value, "cat")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						request.app.content.cat.push_back(temp->text);
						temp = temp->next;
					}
				}

				if((label = json_find_first_label(content_value, "videoquality")) != NULL)
					request.app.content.videoquality = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "keywords")) != NULL)
					request.app.content.keywords = label->child->text;

				if((label = json_find_first_label(content_value, "contentrating")) != NULL)
					request.app.content.contentrating = label->child->text;

				if((label = json_find_first_label(content_value, "userrating")) != NULL)
					request.app.content.userrating = label->child->text;

				if((label = json_find_first_label(content_value, "context")) != NULL)
					request.app.content.context = label->child->text;


				if((label = json_find_first_label(content_value, "livestream")) != NULL)
					request.app.content.livestream = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "sourcerelationship")) != NULL)
					request.app.content.sourcerelationship = atoi(label->child->text);

				if((label = json_find_first_label(content_value, "producer")) != NULL)
				{
					json_t *producer_value = label->child;

					if(producer_value != NULL)
					{
						if((label = json_find_first_label(producer_value, "id")) != NULL)
							request.app.content.producer.id = label->child->text;

						if((label = json_find_first_label(producer_value, "name")) != NULL)
							request.app.content.producer.name = label->child->text;

						if ((label = json_find_first_label(producer_value, "cat")) != NULL)
						{
							json_t *temp = label->child->child;

							while (temp != NULL)
							{
								request.app.content.producer.cat.push_back(temp->text);
								temp = temp->next;
							}
						}

						if((label = json_find_first_label(producer_value, "domain")) != NULL)
							request.app.content.producer.domain = label->child->text;
					}
				}

				if((label = json_find_first_label(content_value, "len")) != NULL)
					request.app.content.len = atoi(label->child->text);
			}
		}

		if((label = json_find_first_label(app_child, "keywords")) != NULL)
			request.app.keywords = label->child->text;
	}

	if((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;

		if((label = json_find_first_label(device_child, "dnt")) != NULL && label->child->type == JSON_NUMBER)
			request.device.dnt = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "ua")) != NULL)
			request.device.ua = label->child->text;

		if((label = json_find_first_label(device_child, "ip")) != NULL)
			request.device.ip = label->child->text;

		if((label = json_find_first_label(device_child, "geo")) != NULL)
		{
			json_t *geo_value = label->child;

			if((label = json_find_first_label(geo_value, "lat")) != NULL)
				request.device.geo.lat = atof(label->child->text);// atof to float???

			if((label = json_find_first_label(geo_value, "lon")) != NULL)
				request.device.geo.lat = atof(label->child->text);

			if((label = json_find_first_label(geo_value, "country")) != NULL)
				request.device.geo.country = label->child->text;

			if((label = json_find_first_label(geo_value, "region")) != NULL)
				request.device.geo.region = label->child->text;

			if((label = json_find_first_label(geo_value, "regionfips104")) != NULL)
				request.device.geo.regionfips104 = label->child->text;

			if((label = json_find_first_label(geo_value, "metro")) != NULL)
				request.device.geo.metro = label->child->text;

			if((label = json_find_first_label(geo_value, "city")) != NULL)
				request.device.geo.city = label->child->text;

			if((label = json_find_first_label(geo_value, "zip")) != NULL)
				request.device.geo.zip = label->child->text;

			if((label = json_find_first_label(geo_value, "type")) != NULL && label->child->type == JSON_NUMBER)
				request.device.geo.type = atoi(label->child->text);
		}

		if((label = json_find_first_label(device_child, "didsha1")) != NULL)
			request.device.didsha1 = label->child->text;

		if((label = json_find_first_label(device_child, "didmd5")) != NULL)
			request.device.didmd5 = label->child->text;

		if((label = json_find_first_label(device_child, "dpidsha1")) != NULL)
			request.device.dpidsha1 = label->child->text;

		if((label = json_find_first_label(device_child, "dpidmd5")) != NULL)
			request.device.dpidmd5 = label->child->text;
		
		if((label = json_find_first_label(device_child, "ipv6")) != NULL)
			request.device.ipv6 = label->child->text;

		if((label = json_find_first_label(device_child, "carrier")) != NULL)
			request.device.carrier = label->child->text;

		if((label = json_find_first_label(device_child, "language")) != NULL)
			request.device.language = label->child->text;

		if((label = json_find_first_label(device_child, "make")) != NULL)
			request.device.make = label->child->text;

		if((label = json_find_first_label(device_child, "model")) != NULL)
			request.device.model = label->child->text;

		if((label = json_find_first_label(device_child, "os")) != NULL)
			request.device.os = label->child->text;

		if((label = json_find_first_label(device_child, "osv")) != NULL)
			request.device.osv = label->child->text;

		if((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
			request.device.js = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
			request.device.connectiontype = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
			request.device.devicetype = atoi(label->child->text);

		if((label = json_find_first_label(device_child, "flashver")) != NULL)
			request.device.flashver = label->child->text;

		if((label = json_find_first_label(device_child, "ext")) != NULL)
		{
			json_t *ext_value = label->child;

			if((label = json_find_first_label(ext_value, "idfasha1")) != NULL)
				request.device.ext.idfasha1 = label->child->text;

			if((label = json_find_first_label(ext_value, "idfamd5")) != NULL)
				request.device.ext.idfamd5 = label->child->text;
		}
	}

	if((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
		request.at = atoi(label->child->text);

	if((label = json_find_first_label(root, "tmax")) != NULL && label->child->type == JSON_NUMBER)
			request.tmax = atoi(label->child->text);
	
	if ((label = json_find_first_label(root, "wseat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.wseat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if((label = json_find_first_label(root, "allimps")) != NULL && label->child->type == JSON_NUMBER)
		request.allimps = atoi(label->child->text);

	if ((label = json_find_first_label(root, "cur")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.cur.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "bcat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.bcat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			request.badv.push_back(temp->text);
			temp = temp->next;
		}
	}

	json_free_value(&root);

	return true;
}

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN DATATRANSFER *datatransfer, IN RECVDATA *recvdata, OUT string &senddata, OUT string &requestid)
{
	int errcode = E_SUCCESS;
	MESSAGEREQUEST mrequest;//adx msg request

	string output = "";

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
//		cflog(g_logid_local, LOGERROR, "recvdata->data is null or recvdata->length is 0");
		errcode = E_INVALID_PARAM;
		goto exit;
	}

	init_message_request(mrequest);
	if (!parseInmobiRequest(recvdata, mrequest))
	{
//		cflog(g_logid_local, LOGERROR, "parseInmobiRequest failed!");
		errcode = E_BAD_REQUEST;
		goto exit;
	}

	requestid = mrequest.id;
	if (requestid == "")
	{
	  errcode = E_BAD_REQUEST;
		goto exit;
	}

//	errcode = E_UNKNOWN;

exit:

	if (errcode == E_SUCCESS) 
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}
	
	return errcode;
}




