/*
 * inmobi_response.cpp
 *
 *  Created on: March 20, 2015
 *      Author: root
 */

#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>

#include "../../common/errorcode.h"
#include "../../common/json_util.h"
#include "../../common/bid_filter.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "inmobi_response.h"

extern uint64_t g_logid_local, g_logid_local_traffic, g_logid, g_logid_response;
extern map<string, vector<uint32_t> > app_cat_table_adx2com;
extern map<string, vector<uint32_t> > adv_cat_table_adx2com;
//extern map<uint32_t, vector<string> > adv_cat_table_com2adx;
extern map<string, uint16_t> dev_make_table;
extern MD5_CTX hash_ctx;
extern uint64_t geodb;

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

static void init_user_object(USEROBJECT &user)//
{
	user.yob = -1;
}

static void init_device_object(DEVICEOBJECT &device)//
{
	device.dnt = device.js = device.connectiontype = device.devicetype = 0;

	init_geo_object(device.geo);

	//	device.ua = device.ip = device.didsha1 = device.didmd5 = device.dpidsha1 = device.dpidmd5 = device.ipv6
	//		= device.carrier = device.language = device.make = device.model = device.os = device.osv = device.flashver = "";
}

void init_message_request(MESSAGEREQUEST &mrequest)//
{
	//	mrequest.id = "";

	init_site_object(mrequest.site);

	init_app_object(mrequest.app);

	init_device_object(mrequest.device);

	init_user_object(mrequest.user);

	mrequest.at = 2;

	mrequest.tmax = mrequest.allimps = 0;

	mrequest.is_secure = 0;
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

static void init_title_object(TITLEOBJECT &title)
{
	title.len = 0;
}

static void init_img_object(IMGOBJECT &img)
{
	img.type = img.w = img.wmin = img.h = img.hmin = 0;
}

static void init_data_object(DATAOBJECT &data)
{
	data.type = data.len = 0;
}

static void init_asset_request(ASSETOBJECT &asset)
{
	asset.id = asset.required = asset.type = 0;
	init_title_object(asset.title);
	init_img_object(asset.img);
	init_data_object(asset.data);
}

static void init_native_request(NATIVEREQUEST &natreq)
{
	natreq.layout = natreq.adunit = natreq.plcmtnt = natreq.seq = 0;
}

static void init_native_object(NATIVEOBJECT &native)
{
	init_native_request(native.requestobj);
}

static void init_deal_object(DEALOBJECT &deal)
{
	deal.bidfloor = 0;
	deal.at = 0;
}

static void init_pmp_object(PMPOBJECT &pmp)
{
	pmp.private_auction = 0;
}

static void init_impression_object(IMPRESSIONOBJECT &imp)
{
	//	imp.id = imp.tagid = "";

	//	imp.bidfloorcur = "";

	imp.instl = imp.bidfloor = 0;

	init_banner_object(imp.banner);

	init_native_object(imp.native);

	init_pmp_object(imp.pmp);
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
	//	response.cur = "USD";//default
}

bool parse_inmobi_request(char *recvdata, MESSAGEREQUEST &request, int &imptype)
{
	json_t *root = NULL;
	json_t *label = NULL;

	json_parse_document(&root, recvdata);
	if (root == NULL)
		return false;

	if ((label = json_find_first_label(root, "id")) != NULL)
		request.id = label->child->text;

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;   // array,get first

		if (imp_value != NULL)
		{
			IMPRESSIONOBJECT imp;

			init_impression_object(imp);

			if ((label = json_find_first_label(imp_value, "id")) != NULL)
				imp.id = label->child->text;

			if ((label = json_find_first_label(imp_value, "secure")) != NULL &&
				label->child->type == JSON_NUMBER)
				request.is_secure = atoi(label->child->text);

			if ((label = json_find_first_label(imp_value, "banner")) != NULL)
			{
				json_t *banner_value = label->child;

				if (banner_value != NULL)
				{
					//BANNEROBJECT bannerobject;

					imptype = IMP_BANNER;

					if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.w = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.h = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "id")) != NULL)
						imp.banner.id = label->child->text;

					if ((label = json_find_first_label(banner_value, "pos")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.pos = atoi(label->child->text);

					/*
					//the meaning of inmobi is not equal to pxene defined type, so ignore.
					if ((label = json_find_first_label(banner_value, "btype")) != NULL)
					{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
					imp.banner.btype.push_back(atoi(temp->text));
					temp = temp->next;
					}
					}

					//inmobi said: the attr has no use, just ignore currently.
					if ((label = json_find_first_label(banner_value, "battr")) != NULL)
					{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
					imp.banner.battr.push_back(atoi(temp->text));
					temp = temp->next;
					}
					}
					*/

					if ((label = json_find_first_label(banner_value, "topframe")) != NULL && label->child->type == JSON_NUMBER)
						imp.banner.topframe = atoi(label->child->text);

					if ((label = json_find_first_label(banner_value, "ext")) != NULL)
					{
						json_t *ext_value = label->child;
						if ((label = json_find_first_label(ext_value, "adh")) != NULL && label->child->type == JSON_NUMBER)
							imp.banner.ext.adh = atoi(label->child->text);
						if ((label = json_find_first_label(ext_value, "orientation")) != NULL)
							imp.banner.ext.orientation = label->child->text;
						if ((label = json_find_first_label(ext_value, "orientationlock")) != NULL && label->child->type == JSON_NUMBER)
							imp.banner.ext.orientationlock = atoi(label->child->text);
					}
				}
			}

			if ((label = json_find_first_label(imp_value, "pmp")) != NULL)
			{
				json_t *pmp_value = label->child;
				if (pmp_value != NULL)
				{
					if ((label = json_find_first_label(pmp_value, "private_auction")) != NULL && label->child->type == JSON_NUMBER)
						imp.pmp.private_auction = atoi(label->child->text);
					if ((label = json_find_first_label(pmp_value, "deals")) != NULL)
					{
						json_t *temp = label->child->child;

						while (temp != NULL)
						{
							DEALOBJECT deal;
							init_deal_object(deal);
							if ((label = json_find_first_label(temp, "id")) != NULL)
								deal.id = label->child->text;
							if ((label = json_find_first_label(temp, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
								deal.bidfloor = atof(label->child->text);
							if ((label = json_find_first_label(temp, "bidfloorcur")) != NULL)
								deal.bidfloorcur = label->child->text;
							if ((label = json_find_first_label(temp, "at")) != NULL && label->child->type == JSON_NUMBER)
								deal.at = atoi(label->child->text);

							if (deal.id != "")
							{
								imp.pmp.deals.push_back(deal);
							}
							temp = temp->next;
						}
					}
				}
			}

			if ((label = json_find_first_label(imp_value, "native")) != NULL)
			{
				json_t *native_value = label->child;

				if (native_value != NULL)
				{
					imptype = IMP_NATIVE;
					if ((label = json_find_first_label(native_value, "requestobj")) != NULL)
					{
						json_t *requestobj_value = label->child;
						if (requestobj_value != NULL)
						{
							if ((label = json_find_first_label(requestobj_value, "ver")) != NULL)
							{
								imp.native.requestobj.ver = label->child->text;
							}

							if ((label = json_find_first_label(requestobj_value, "layout")) != NULL && label->child->type == JSON_NUMBER)
							{
								imp.native.requestobj.layout = atoi(label->child->text);
							}

							if ((label = json_find_first_label(requestobj_value, "adunit")) != NULL && label->child->type == JSON_NUMBER)
							{
								imp.native.requestobj.adunit = atoi(label->child->text);
								writeLog(g_logid_local, LOGDEBUG, "id=%s,pid=%d", request.id.c_str(), imp.native.requestobj.adunit);
							}

							if ((label = json_find_first_label(requestobj_value, "plcmtcnt")) != NULL && label->child->type == JSON_NUMBER)
							{
								imp.native.requestobj.plcmtnt = atoi(label->child->text);
							}

							if ((label = json_find_first_label(requestobj_value, "seq")) != NULL && label->child->type == JSON_NUMBER)
							{
								imp.native.requestobj.seq = atoi(label->child->text);
							}

							if ((label = json_find_first_label(requestobj_value, "assets")) != NULL)
							{
								json_t *asset_value = label->child->child;

								while (asset_value != NULL)
								{
									ASSETOBJECT asset;
									init_asset_request(asset);
									if ((label = json_find_first_label(asset_value, "id")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.id = atoi(label->child->text);
									}

									if ((label = json_find_first_label(asset_value, "required")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.required = atoi(label->child->text);
									}

									if ((label = json_find_first_label(asset_value, "title")) != NULL)
									{
										json_t *title_value = label->child;
										if (title_value != NULL)
										{
											asset.type = NATIVE_ASSETTYPE_TITLE;
											if ((label = json_find_first_label(title_value, "len")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.title.len = atoi(label->child->text);
											}
										}
									}

									if ((label = json_find_first_label(asset_value, "img")) != NULL)
									{
										json_t *img_value = label->child;
										if (img_value != NULL)
										{
											asset.type = NATIVE_ASSETTYPE_IMAGE;
											if ((label = json_find_first_label(img_value, "type")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.img.type = atoi(label->child->text);
											}

											if ((label = json_find_first_label(img_value, "w")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.img.w = atoi(label->child->text);
											}

											if ((label = json_find_first_label(img_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.img.wmin = atoi(label->child->text);
											}

											if ((label = json_find_first_label(img_value, "h")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.img.h = atoi(label->child->text);
											}

											if ((label = json_find_first_label(img_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.img.hmin = atoi(label->child->text);
											}

											if ((label = json_find_first_label(img_value, "mimes")) != NULL)
											{
												json_t *tmp = label->child->child;
												while (tmp != NULL)
												{
													asset.img.mimes.push_back(label->text);
													tmp = tmp->next;
												}
											}
										}
									}

									if ((label = json_find_first_label(asset_value, "data")) != NULL)
									{
										json_t *data_value = label->child;
										if (data_value != NULL)
										{
											asset.type = NATIVE_ASSETTYPE_DATA;
											if ((label = json_find_first_label(data_value, "type")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.data.type = atoi(label->child->text);
											}

											if ((label = json_find_first_label(data_value, "len")) != NULL && label->child->type == JSON_NUMBER)
											{
												asset.data.len = atoi(label->child->text);
											}
										}
									}

									imp.native.requestobj.assets.push_back(asset);
									asset_value = asset_value->next;
								}
							}
						}
					}

					if ((label = json_find_first_label(native_value, "ver")) != NULL)
					{
						imp.native.ver = label->child->text;
					}

					if ((label = json_find_first_label(native_value, "api")) != NULL)
					{
						json_t *temp = label->child->child;

						while (temp != NULL)
						{
							imp.native.api.push_back(atoi(temp->text));
							temp = temp->next;
						}
					}
				}
			}


			if ((label = json_find_first_label(imp_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
				imp.instl = atoi(label->child->text);

			if ((label = json_find_first_label(imp_value, "tagid")) != NULL)
				imp.tagid = label->child->text;

			if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
			{
				imp.bidfloor = atof(label->child->text);
			}
			//else
			//	cout<<"not find imp.bidfloor: "<<imp.bidfloor<<" !!!"<<endl;

			if ((label = json_find_first_label(imp_value, "bidfloorcur")) != NULL)
				imp.bidfloorcur = label->child->text;

			if ((label = json_find_first_label(imp_value, "ext")) != NULL)
			{
			}

			request.imp.push_back(imp);
		}
	}

	if ((label = json_find_first_label(root, "site")) != NULL)
	{
		json_t *site_child = label->child;

		if ((label = json_find_first_label(site_child, "id")) != NULL)
			request.site.id = label->child->text;

		if ((label = json_find_first_label(site_child, "name")) != NULL)
			request.site.name = label->child->text;

		if ((label = json_find_first_label(site_child, "domain")) != NULL)
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

		if ((label = json_find_first_label(site_child, "privacypolicy")) != NULL && label->child->type == JSON_NUMBER)
			request.site.privacypolicy = atoi(label->child->text);

		if ((label = json_find_first_label(site_child, "ref")) != NULL)
			request.site.ref = label->child->text;

		if ((label = json_find_first_label(site_child, "search")) != NULL)
			request.site.search = label->child->text;

		//pub
		if ((label = json_find_first_label(site_child, "publisher")) != NULL)
		{
			json_t *publisher_value = label->child;

			if (publisher_value != NULL)
			{
				if ((label = json_find_first_label(publisher_value, "id")) != NULL)
					request.site.publisher.id = label->child->text;

				if ((label = json_find_first_label(publisher_value, "name")) != NULL)
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

				if ((label = json_find_first_label(publisher_value, "domain")) != NULL)
					request.site.publisher.domain = label->child->text;
			}
		}

		//content
		if ((label = json_find_first_label(site_child, "content")) != NULL)
		{
			json_t *content_value = label->child;

			if (content_value != NULL)
			{
				if ((label = json_find_first_label(content_value, "id")) != NULL)
					request.site.content.id = label->child->text;

				if ((label = json_find_first_label(content_value, "episode")) != NULL && label->child->type == JSON_NUMBER)
					request.site.content.episode = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "title")) != NULL)
					request.site.content.title = label->child->text;

				if ((label = json_find_first_label(content_value, "series")) != NULL)
					request.site.content.series = label->child->text;

				if ((label = json_find_first_label(content_value, "season")) != NULL)
					request.site.content.season = label->child->text;

				if ((label = json_find_first_label(content_value, "url")) != NULL)
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

				if ((label = json_find_first_label(content_value, "videoquality")) != NULL && label->child->type == JSON_NUMBER)
					request.site.content.videoquality = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "keywords")) != NULL)
					request.site.content.keywords = label->child->text;

				if ((label = json_find_first_label(content_value, "contentrating")) != NULL)
					request.site.content.contentrating = label->child->text;

				if ((label = json_find_first_label(content_value, "userrating")) != NULL)
					request.site.content.userrating = label->child->text;

				if ((label = json_find_first_label(content_value, "context")) != NULL)
					request.site.content.context = label->child->text;


				if ((label = json_find_first_label(content_value, "livestream")) != NULL && label->child->type == JSON_NUMBER)
					request.site.content.livestream = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "sourcerelationship")) != NULL && label->child->type == JSON_NUMBER)
					request.site.content.sourcerelationship = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "producer")) != NULL)
				{
					json_t *producer_value = label->child;

					if (producer_value != NULL)
					{
						if ((label = json_find_first_label(producer_value, "id")) != NULL)
							request.site.content.producer.id = label->child->text;

						if ((label = json_find_first_label(producer_value, "name")) != NULL)
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

						if ((label = json_find_first_label(producer_value, "domain")) != NULL)
							request.site.content.producer.domain = label->child->text;
					}
				}

				if ((label = json_find_first_label(content_value, "len")) != NULL && label->child->type == JSON_NUMBER)
					request.site.content.len = atoi(label->child->text);
			}
		}

		if ((label = json_find_first_label(site_child, "keywords")) != NULL)
			request.site.keywords = label->child->text;
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if ((label = json_find_first_label(app_child, "id")) != NULL)
			request.app.id = label->child->text;

		if ((label = json_find_first_label(app_child, "name")) != NULL)
			request.app.name = label->child->text;

		if ((label = json_find_first_label(app_child, "domain")) != NULL)
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

		if ((label = json_find_first_label(app_child, "ver")) != NULL)
			request.app.ver = label->child->text;

		if ((label = json_find_first_label(app_child, "bundle")) != NULL)
			request.app.bundle = label->child->text;

		if ((label = json_find_first_label(app_child, "privacypolicy")) != NULL && label->child->type == JSON_NUMBER)
			request.app.privacypolicy = atoi(label->child->text);

		if ((label = json_find_first_label(app_child, "paid")) != NULL && label->child->type == JSON_NUMBER)
			request.app.paid = atoi(label->child->text);

		//pub
		if ((label = json_find_first_label(app_child, "publisher")) != NULL)
		{
			json_t *publisher_value = label->child;

			if (publisher_value != NULL)
			{
				if ((label = json_find_first_label(publisher_value, "id")) != NULL)
					request.app.publisher.id = label->child->text;

				if ((label = json_find_first_label(publisher_value, "name")) != NULL)
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

				if ((label = json_find_first_label(publisher_value, "domain")) != NULL)
					request.app.publisher.domain = label->child->text;
			}
		}

		//content
		if ((label = json_find_first_label(app_child, "content")) != NULL)
		{
			json_t *content_value = label->child;

			if (content_value != NULL)
			{
				if ((label = json_find_first_label(content_value, "id")) != NULL)
					request.app.content.id = label->child->text;

				if ((label = json_find_first_label(content_value, "episode")) != NULL && label->child->type == JSON_NUMBER)
					request.app.content.episode = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "title")) != NULL)
					request.app.content.title = label->child->text;

				if ((label = json_find_first_label(content_value, "series")) != NULL)
					request.app.content.series = label->child->text;

				if ((label = json_find_first_label(content_value, "season")) != NULL)
					request.app.content.season = label->child->text;

				if ((label = json_find_first_label(content_value, "url")) != NULL)
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

				if ((label = json_find_first_label(content_value, "videoquality")) != NULL && label->child->type == JSON_NUMBER)
					request.app.content.videoquality = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "keywords")) != NULL)
					request.app.content.keywords = label->child->text;

				if ((label = json_find_first_label(content_value, "contentrating")) != NULL)
					request.app.content.contentrating = label->child->text;

				if ((label = json_find_first_label(content_value, "userrating")) != NULL)
					request.app.content.userrating = label->child->text;

				if ((label = json_find_first_label(content_value, "context")) != NULL)
					request.app.content.context = label->child->text;


				if ((label = json_find_first_label(content_value, "livestream")) != NULL && label->child->type == JSON_NUMBER)
					request.app.content.livestream = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "sourcerelationship")) != NULL && label->child->type == JSON_NUMBER)
					request.app.content.sourcerelationship = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "producer")) != NULL)
				{
					json_t *producer_value = label->child;

					if (producer_value != NULL)
					{
						if ((label = json_find_first_label(producer_value, "id")) != NULL)
							request.app.content.producer.id = label->child->text;

						if ((label = json_find_first_label(producer_value, "name")) != NULL)
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

						if ((label = json_find_first_label(producer_value, "domain")) != NULL)
							request.app.content.producer.domain = label->child->text;
					}
				}

				if ((label = json_find_first_label(content_value, "len")) != NULL && label->child->type == JSON_NUMBER)
					request.app.content.len = atoi(label->child->text);
			}
		}

		if ((label = json_find_first_label(app_child, "keywords")) != NULL)
			request.app.keywords = label->child->text;

		if ((label = json_find_first_label(app_child, "storeurl")) != NULL)
			request.app.storeurl = label->child->text;
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;

		if (device_child != NULL)
		{
			if ((label = json_find_first_label(device_child, "dnt")) != NULL && label->child->type == JSON_NUMBER)
				request.device.dnt = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "ua")) != NULL)
				request.device.ua = label->child->text;

			if ((label = json_find_first_label(device_child, "ip")) != NULL)
				request.device.ip = label->child->text;

			if ((label = json_find_first_label(device_child, "geo")) != NULL)
			{
				json_t *geo_value = label->child;

				if ((label = json_find_first_label(geo_value, "lat")) != NULL && label->child->type == JSON_NUMBER)
					request.device.geo.lat = atof(label->child->text);// atof to float???

				if ((label = json_find_first_label(geo_value, "lon")) != NULL && label->child->type == JSON_NUMBER)
					request.device.geo.lon = atof(label->child->text);

				if ((label = json_find_first_label(geo_value, "country")) != NULL)
					request.device.geo.country = label->child->text;

				if ((label = json_find_first_label(geo_value, "region")) != NULL)
					request.device.geo.region = label->child->text;

				if ((label = json_find_first_label(geo_value, "regionfips104")) != NULL)
					request.device.geo.regionfips104 = label->child->text;

				if ((label = json_find_first_label(geo_value, "metro")) != NULL)
					request.device.geo.metro = label->child->text;

				if ((label = json_find_first_label(geo_value, "city")) != NULL)
					request.device.geo.city = label->child->text;

				if ((label = json_find_first_label(geo_value, "zip")) != NULL)
					request.device.geo.zip = label->child->text;

				if ((label = json_find_first_label(geo_value, "type")) != NULL && label->child->type == JSON_NUMBER)
					request.device.geo.type = atoi(label->child->text);
			}

			//		if((label = json_find_first_label(device_child, "did")) != NULL)
			// 			request.device.did = label->child->text;

			if ((label = json_find_first_label(device_child, "didsha1")) != NULL)
				request.device.didsha1 = label->child->text;

			if ((label = json_find_first_label(device_child, "didmd5")) != NULL)
				request.device.didmd5 = label->child->text;

			if ((label = json_find_first_label(device_child, "dpid")) != NULL)
				request.device.dpid = label->child->text;

			if ((label = json_find_first_label(device_child, "dpidsha1")) != NULL)
				request.device.dpidsha1 = label->child->text;

			if ((label = json_find_first_label(device_child, "dpidmd5")) != NULL)
				request.device.dpidmd5 = label->child->text;

			if ((label = json_find_first_label(device_child, "ipv6")) != NULL)
				request.device.ipv6 = label->child->text;

			if ((label = json_find_first_label(device_child, "carrier")) != NULL)
				request.device.carrier = label->child->text;

			if ((label = json_find_first_label(device_child, "language")) != NULL)
				request.device.language = label->child->text;

			if ((label = json_find_first_label(device_child, "make")) != NULL)
				request.device.make = label->child->text;

			if ((label = json_find_first_label(device_child, "model")) != NULL)
				request.device.model = label->child->text;

			if ((label = json_find_first_label(device_child, "os")) != NULL)
				request.device.os = label->child->text;

			if ((label = json_find_first_label(device_child, "osv")) != NULL)
				request.device.osv = label->child->text;

			if ((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
				request.device.js = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
				request.device.connectiontype = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
				request.device.devicetype = atoi(label->child->text);

			if ((label = json_find_first_label(device_child, "flashver")) != NULL)
				request.device.flashver = label->child->text;

			if ((label = json_find_first_label(device_child, "ext")) != NULL)
			{
				json_t *ext_value = label->child;

				if ((label = json_find_first_label(ext_value, "idfa")) != NULL)
					request.device.ext.idfa = label->child->text;

				if ((label = json_find_first_label(ext_value, "idfasha1")) != NULL)
					request.device.ext.idfasha1 = label->child->text;

				if ((label = json_find_first_label(ext_value, "idfamd5")) != NULL)
					request.device.ext.idfamd5 = label->child->text;

				if ((label = json_find_first_label(ext_value, "gpid")) != NULL)
					request.device.ext.gpid = label->child->text;
			}
		}
	}

	if ((label = json_find_first_label(root, "user")) != NULL)
	{
		json_t *user_child = label->child;

		if (user_child != NULL)
		{
			if ((label = json_find_first_label(user_child, "id")) != NULL)
				request.user.id = label->child->text;

			if ((label = json_find_first_label(user_child, "yob")) != NULL && label->child->type == JSON_NUMBER)
				request.user.yob = atoi(label->child->text);

			if ((label = json_find_first_label(user_child, "gender")) != NULL)
				request.user.gender = label->child->text;
		}
	}

	if ((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
		request.at = atoi(label->child->text);

	if ((label = json_find_first_label(root, "tmax")) != NULL && label->child->type == JSON_NUMBER)
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

	if ((label = json_find_first_label(root, "allimps")) != NULL && label->child->type == JSON_NUMBER)
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

static bool setInmobiJsonResponse(MESSAGERESPONSE &response, int imptype, string &response_out)
{
	if (imptype == IMP_UNKNOWN)
		return false;
	char *text = NULL;
	json_t *root, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;

	root = json_new_object();
	if (root == NULL)
		return false;
	jsonInsertString(root, "id", response.id.c_str());
	if (!response.seatbid.empty())
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < response.seatbid.size(); i++)
		{
			char *seatbid_text;
			SEATBIDOBJECT *seatbid = &response.seatbid[i];
			entry_seatbid = json_new_object();
			if (seatbid->bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid->bid.size(); j++)
				{
					char *bid_text;
					BIDOBJECT *bid = &seatbid->bid[j];
					entry_bid = json_new_object();

					jsonInsertString(entry_bid, "id", bid->id.c_str());
					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					jsonInsertFloat(entry_bid, "price", bid->price, 6);//%.6lf
					jsonInsertString(entry_bid, "adid", bid->adid.c_str());
					jsonInsertString(entry_bid, "nurl", bid->nurl.c_str());

					if (bid->dealid != "")
					{
						jsonInsertString(entry_bid, "dealid", bid->dealid.c_str());
					}

					if (imptype == IMP_BANNER)
						jsonInsertString(entry_bid, "adm", bid->adm.c_str());
					else if (imptype == IMP_NATIVE)
					{
						json_t *entry_admobject = json_new_object();
						json_t *entry_native = json_new_object();

						// native
						json_t *array_imptr = json_new_array();
						json_t *value_imptr;
						for (int k = 0; k < bid->admobject.native.imptrackers.size(); ++k)
						{
							value_imptr = json_new_string(bid->admobject.native.imptrackers[k].c_str());
							json_insert_child(array_imptr, value_imptr);
						}
						json_insert_pair_into_object(entry_native, "imptrackers", array_imptr);

						//link
						json_t *entry_link = json_new_object();
						jsonInsertString(entry_link, "url", bid->admobject.native.link.url.c_str());
						//clickurl
						json_t *array_clk = json_new_array();
						json_t *value_clk;
						for (int k = 0; k < bid->admobject.native.link.clicktrackers.size(); ++k)
						{
							value_clk = json_new_string(bid->admobject.native.link.clicktrackers[k].c_str());
							json_insert_child(array_clk, value_clk);
						}
						json_insert_pair_into_object(entry_link, "clicktrackers", array_clk);
						json_insert_pair_into_object(entry_native, "link", entry_link);

						//assets
						json_t *array_assets = json_new_array();

						for (int k = 0; k < bid->admobject.native.assets.size(); ++k)
						{
							ASSETRESPONSE &asset_rep = bid->admobject.native.assets[k];
							json_t *entry_assets = json_new_object();
							jsonInsertInt(entry_assets, "id", asset_rep.id);
							jsonInsertInt(entry_assets, "required", asset_rep.required);

							if (asset_rep.type == NATIVE_ASSETTYPE_TITLE)
							{
								json_t *title = json_new_object();
								jsonInsertString(title, "text", asset_rep.title.text.c_str());
								json_insert_pair_into_object(entry_assets, "title", title);
							}
							else if (asset_rep.type == NATIVE_ASSETTYPE_IMAGE)
							{
								json_t *img = json_new_object();
								jsonInsertString(img, "url", asset_rep.img.url.c_str());
								jsonInsertInt(img, "w", asset_rep.img.w);
								jsonInsertInt(img, "h", asset_rep.img.h);
								json_insert_pair_into_object(entry_assets, "img", img);
							}
							else if (asset_rep.type == NATIVE_ASSETTYPE_DATA)
							{
								json_t *data = json_new_object();
								jsonInsertString(data, "label", asset_rep.data.label.c_str());
								jsonInsertString(data, "value", asset_rep.data.value.c_str());
								json_insert_pair_into_object(entry_assets, "data", data);
							}
							json_insert_child(array_assets, entry_assets);
						}
						json_insert_pair_into_object(entry_native, "assets", array_assets);
						json_insert_pair_into_object(entry_admobject, "native", entry_native);
						json_insert_pair_into_object(entry_bid, "admobject", entry_admobject);
					}

					if (bid->adomain.size() > 0)
					{
						json_t *label_adomain, *array_adomain, *value_adomain;

						label_adomain = json_new_string("adomain");
						array_adomain = json_new_array();
						for (int k = 0; k < bid->adomain.size(); k++)
						{
							value_adomain = json_new_string(bid->adomain[k].c_str());
							json_insert_child(array_adomain, value_adomain);
						}
						json_insert_child(label_adomain, array_adomain);
						json_insert_child(entry_bid, label_adomain);
					}

					jsonInsertString(entry_bid, "iurl", bid->iurl.c_str());
					jsonInsertString(entry_bid, "cid", bid->cid.c_str());
					jsonInsertString(entry_bid, "crid", bid->crid.c_str());

					if (bid->attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						for (int k = 0; k < bid->attr.size(); k++)
						{
							char buf[16] = "";
							sprintf(buf, "%d", bid->attr[k]);
							value_attr = json_new_number(buf);
							json_insert_child(array_attr, value_attr);
						}
						json_insert_child(label_attr, array_attr);
						json_insert_child(entry_bid, label_attr);

					}
					json_insert_child(array_bid, entry_bid);

				}
				json_insert_child(label_bid, array_bid);
				json_insert_child(entry_seatbid, label_bid);

			}
			if (seatbid->seat.size() > 0)
				jsonInsertString(entry_seatbid, "seat", seatbid->seat.c_str());

			json_insert_child(array_seatbid, entry_seatbid);
		}
		json_insert_child(label_seatbid, array_seatbid);
		json_insert_child(root, label_seatbid);
	}

	jsonInsertString(root, "bidid", response.bidid.c_str());

	jsonInsertString(root, "cur", response.cur.c_str());

	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		response_out = text;
		free(text);
		text = NULL;
	}

	json_free_value(&root);
	return true;
}
/*
uint64_t iab_to_uint64(string cat)
{
int icat = 0, isubcat = 0;
sscanf(cat.c_str(), "IAB%x-%x", &icat, &isubcat);
return (icat << 8) | isubcat;
}

void callback_appcat_insert_pair_range(IN void *data, const string key_start, const string key_end, const string val)
{
map<uint32_t, vector<uint32_t> > &table = *((map<uint32_t, vector<uint32_t> > *)data);

uint32_t com_cat;
sscanf(val.c_str(), "0x%x", &com_cat);//0x%x

uint64_t u64_start = iab_to_uint64(key_start);
uint64_t u64_end = iab_to_uint64(key_end);

assert(u64_start <= u64_end);

for(uint64_t i = u64_start; i <= u64_end; ++i)
{
vector<uint32_t> &s_val = table[i];

s_val.push_back(com_cat);

va_cout("%s: insert: 0x%x -> 0x%x", __FUNCTION__, i, com_cat);
}
}
*/

void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	//	va_cout("%s, key_start:%s, val:%s, com_cat:0x%x", __FUNCTION__, key_start.c_str(), val.c_str(), com_cat);

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

void callback_insert_pair_hex_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<string> > &table = *((map<uint32_t, vector<string> > *)data);

	uint32_t com_cat;
	sscanf(key_start.c_str(), "%x", &com_cat);//0x%x

	//	va_cout("%s, key_start:%s, val:%s, com_cat:0x%x", __FUNCTION__, key_start.c_str(), val.c_str(), com_cat);

	vector<string> &s_val = table[com_cat];

	s_val.push_back(val);
}

string transform_advcat_com_to_adx(uint16_t cat)
{
	return "";
}

uint8_t transform_carrier(string carrier)
{
	if (carrier == "China Mobile")
		return CARRIER_CHINAMOBILE;

	if (carrier == "China Unicom")
		return CARRIER_CHINAUNICOM;

	if (carrier == "China Telecom")
		return CARRIER_CHINATELECOM;

	return CARRIER_UNKNOWN;
}

uint8_t transform_connectiontype(uint8_t connectiontype)
{
	uint8_t com_connectiontype = CONNECTION_UNKNOWN;

	switch (connectiontype)
	{
	case 2:
		com_connectiontype = CONNECTION_WIFI;
		break;
	case 3:
		com_connectiontype = CONNECTION_CELLULAR_UNKNOWN;
		break;
	case 4:
		com_connectiontype = CONNECTION_CELLULAR_2G;
		break;
	case 5:
		com_connectiontype = CONNECTION_CELLULAR_3G;
		break;
	case 6:
		com_connectiontype = CONNECTION_CELLULAR_4G;
		break;

	default:
		com_connectiontype = CONNECTION_UNKNOWN;
		break;
	}

	return com_connectiontype;
}

uint8_t transform_devicetype(uint8_t devicetype)
{
	uint8_t com_devicetype = DEVICE_UNKNOWN;

	switch (devicetype)
	{
		//	case 1://Mobile/Tablet v2.0
		//		com_devicetype = DEVICE_PHONE;
		//		break;
		//	case 2://Personal Computer v2.0
		//		break;
		//	case 3://Connected TV v2.0
		//		com_devicetype = DEVICE_TV;
		//		break;
	case 4://Phone v2.2
		com_devicetype = DEVICE_PHONE;
		break;
	case 5://Tablet v2.2
		com_devicetype = DEVICE_TABLET;
		break;

	default:
		com_devicetype = DEVICE_UNKNOWN;
		break;
	}

	return com_devicetype;
}
//暂时无法转换.概念不同，故忽略btype
uint8_t transform_type(uint8_t type)
{
	return 0;
}

uint8_t transform_attr(uint8_t attr)
{
	return attr;
}

uint8_t reverse_transform_attr(uint8_t attr)
{
	return transform_attr(attr);
}

void transform_native_mimes(IN vector<string> &inmimes, OUT set<uint8_t> &outmimes)
{
	for (int i = 0; i < inmimes.size(); ++i)
	{
		string temp = inmimes[i];
		if (temp.find("jpg") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_JPG);
		else if (temp.find("jpeg") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_JPG);
		else if (temp.find("png") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_PNG);
		else if (temp.find("gif") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_GIF);
	}
}

int transform_request(IN MESSAGEREQUEST &m, IN int imptype, OUT COM_REQUEST &c)
{
	int i, j;
	int errcode = E_SUCCESS;
	//id
	c.id = m.id;

	c.is_secure = m.is_secure;

	//imp
	for (i = 0; i < m.imp.size(); i++)
	{
		IMPRESSIONOBJECT &imp = m.imp[i];
		COM_IMPRESSIONOBJECT cimp;

		init_com_impression_object(&cimp);

		cimp.id = imp.id;
		cimp.type = imptype;
		if (imptype == IMP_BANNER)
		{
			cimp.banner.w = imp.banner.w;
			cimp.banner.h = imp.banner.h;

			//for (j = 0; j < imp.banner.btype.size(); j++)
			//{
			//	//the meaning of inmobi is not equal to pxene defined type, so ignore.
			//	cimp.banner.btype.insert(transform_type(imp.banner.btype[j]));
			//}

			//for (j = 0; j < imp.banner.battr.size(); j++)
			//{
			//	//inmobi said: the attr has no use, just ignore currently.
			//	cimp.banner.battr.insert(transform_attr(imp.banner.battr[j]));
			//}
		}
		else if (imptype == IMP_NATIVE)
		{
			cimp.native.layout = imp.native.requestobj.layout;
			if (cimp.native.layout != NATIVE_LAYOUTTYPE_NEWSFEED &&
				cimp.native.layout != NATIVE_LAYOUTTYPE_CONTENTSTREAM &&
				cimp.native.layout != NATIVE_LAYOUTTYPE_OPENSCREEN)
			{
				errcode = E_REQUEST_IMP_NATIVE_LAYOUT;
			}

			cimp.native.plcmtcnt = imp.native.requestobj.plcmtnt;
			for (uint32_t k = 0; k < imp.native.requestobj.assets.size(); ++k)
			{
				ASSETOBJECT &asset = imp.native.requestobj.assets[k];
				COM_ASSET_REQUEST_OBJECT asset_request;
				init_com_asset_request_object(asset_request);
				asset_request.id = asset.id;
				asset_request.required = asset.required;
				asset_request.type = asset.type;
				switch (asset.type)
				{
				case NATIVE_ASSETTYPE_TITLE:
					asset_request.title.len = asset.title.len;
					break;
				case NATIVE_ASSETTYPE_IMAGE:
					asset_request.img.type = asset.img.type;  // 这个inmobi的说明和openrtb不符合，联调时需确定?
					if (asset.img.w != 0 && asset.img.h != 0) // inmobi 经常传递的是wmin和hmin
					{

						asset_request.img.w = asset.img.w;
						asset_request.img.wmin = asset.img.wmin;
						asset_request.img.h = asset.img.h;
						asset_request.img.hmin = asset.img.hmin;
					}
					else if (asset.img.wmin != 0 && asset.img.hmin != 0)
					{
						asset_request.img.w = asset.img.wmin;
						asset_request.img.wmin = asset.img.w;
						asset_request.img.h = asset.img.hmin;
						asset_request.img.hmin = asset.img.h;
					}

					transform_native_mimes(asset.img.mimes, asset_request.img.mimes);
					break;
				case NATIVE_ASSETTYPE_DATA:
					asset_request.data.type = asset.data.type;
					if (asset_request.data.type != ASSET_DATATYPE_DESC &&
						asset_request.data.type != ASSET_DATATYPE_RATING &&
						asset_request.data.type != ASSET_DATATYPE_CTATEXT)
					{
						errcode = E_REQUEST_IMP_NATIVE_ASSET_DATA_TYPE;
					}

					asset_request.data.len = asset.data.len;
					break;
				default:
					break;
				}
				cimp.native.assets.push_back(asset_request);
			}
			if (cimp.native.assets.size() == 0)
				errcode = E_REQUEST_IMP_NATIVE_ASSETCNT;
		}

		if (imp.pmp.deals.size() > 0)
		{

			for (int k = 0; k < imp.pmp.deals.size(); ++k)
			{
				DEALOBJECT &deal = imp.pmp.deals[k];
				if (deal.id != "" && deal.bidfloorcur == "CNY")
				{
					cimp.dealprice.insert(make_pair(deal.id, deal.bidfloor));
				}
			}

			c.at = 1;
			if (cimp.dealprice.size() == 0)
			{
				errcode = E_REQUEST_PMP_DEALID;
			}
		}
		cimp.bidfloor = imp.bidfloor;

		cimp.bidfloorcur = imp.bidfloorcur;

		c.imp.push_back(cimp);

		break;
	}

	//app
	c.app.id = m.app.id;
	c.app.name = m.app.name;

	for (i = 0; i < m.app.cat.size(); i++)
	{
		vector<uint32_t> &v_cat = app_cat_table_adx2com[m.app.cat[i]];
		//		va_cout("find app in:%s -> v_cat.size:%d", m.app.cat[i].c_str(), v_cat.size());
		for (j = 0; j < v_cat.size(); j++)
		{
			c.app.cat.insert(v_cat[j]);
		}
	}
	c.app.bundle = m.app.bundle;
	c.app.storeurl = m.app.storeurl;

	//device
	if (m.device.os == "iOS")
	{
		c.device.os = OS_IOS;

		if (m.device.ext.idfa != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA, m.device.ext.idfa));
		}

		if (m.device.ext.idfasha1 != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA_SHA1, m.device.ext.idfasha1));
		}

		if (m.device.ext.idfamd5 != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA_MD5, m.device.ext.idfamd5));
		}
	}
	else if (m.device.os == "Android")
	{
		c.device.os = OS_ANDROID;

		if (m.device.dpid != "")
		{
			c.device.dpids.insert(make_pair(DPID_ANDROIDID, m.device.dpid));
		}

		if (m.device.dpidsha1 != "")
		{
			c.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, m.device.dpidsha1));
		}

		if (m.device.dpidmd5 != "")
		{
			c.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, m.device.dpidmd5));
		}
	}

	if (m.device.didmd5 != "")
	{
		c.device.dids.insert(make_pair(DID_IMEI_MD5, m.device.didmd5));
	}

	if (m.device.didsha1 != "")
	{
		c.device.dids.insert(make_pair(DID_IMEI_SHA1, m.device.didsha1));
	}

	c.device.ua = m.device.ua;
	c.device.ip = m.device.ip;
	if (c.device.ip != "")
	{
		c.device.location = getRegionCode(geodb, c.device.ip);
	}

	c.device.geo.lat = m.device.geo.lat;
	c.device.geo.lon = m.device.geo.lon;
	c.device.carrier = transform_carrier(m.device.carrier);
	//	c.device.make = m.device.make;
	string s_make = m.device.make;
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;

		c.device.makestr = s_make;
		for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				c.device.make = it->second;
				break;
			}
		}
	}

	c.device.model = m.device.model;
	//c.device.os
	c.device.osv = m.device.osv;
	c.device.connectiontype = transform_connectiontype(m.device.connectiontype);
	c.device.devicetype = transform_devicetype(m.device.devicetype);

	// user
	c.user.id = m.user.id;
	c.user.yob = m.user.yob;
	if (m.user.gender == "M")
	{
		c.user.gender = GENDER_MALE;
	}
	else if (m.user.gender == "F")
	{
		c.user.gender = GENDER_FEMALE;
	}
	else
		c.user.gender = GENDER_UNKNOWN;

	//cur
	for (i = 0; i < m.cur.size(); i++)
	{
		c.cur.insert(m.cur[i]);
	}

	//bcat
	for (i = 0; i < m.bcat.size(); i++)
	{
		vector<uint32_t> &v_cat = adv_cat_table_adx2com[m.bcat[i]];
		for (j = 0; j < v_cat.size(); j++)
		{
			c.bcat.insert(v_cat[j]);
		}
	}

	//badv
	for (i = 0; i < m.badv.size(); i++)
	{
		c.badv.insert(m.badv[i]);
	}

	return errcode;
}

void transform_response(IN COM_RESPONSE &c, IN ADXTEMPLATE &adxtemp, IN COM_REQUEST &crequest, OUT MESSAGERESPONSE &m)
{
	m.id = c.id;
	string trace_url_par = string("&") + get_trace_url_par(crequest, c);
	for (int i = 0; i < c.seatbid.size(); i++)
	{
		SEATBIDOBJECT seatbid;
		COM_SEATBIDOBJECT &c_seatbid = c.seatbid[i];

		seatbid.seat = SEAT_INMOBI;

		for (int j = 0; j < c_seatbid.bid.size(); j++)
		{
			BIDOBJECT bid;
			COM_BIDOBJECT &c_bid = c_seatbid.bid[j];
			//replace_https_url(adxtemp,crequest.is_secure);

			bid.id = c_bid.mapid;
			bid.impid = c_bid.impid;
			bid.price = c_bid.price;// 转换
			bid.adid = c_bid.adid;
			bid.dealid = c_bid.dealid;

			if ((bid.nurl = adxtemp.nurl) != "")
			{
				bid.nurl += trace_url_par;
			}

			string iurl;
			if (adxtemp.iurl.size() > 0)
			{
				iurl = adxtemp.iurl[0] + trace_url_par;
			}	

			string curl = c_bid.curl;
			string cturl;
			if (adxtemp.cturl.size() > 0)
			{
				if (c_bid.redirect == 1)
				{
					string curl_temp = curl;

					int en_curl_temp_len = 0;

					char *en_curl_temp = urlencode(curl_temp.c_str(), curl_temp.size(), &en_curl_temp_len);

					string temp_cturl = adxtemp.cturl[0] + trace_url_par;
					curl = temp_cturl + "&curl=" + string(en_curl_temp, en_curl_temp_len);
					cturl = "";
					free(en_curl_temp);
				}
				else
				{
					cturl = adxtemp.cturl[0] + trace_url_par;
				}
			}

			if (crequest.imp[0].type == IMP_BANNER)
			{
				for (int k = 0; k < adxtemp.adms.size(); k++)
				{
					if ((adxtemp.adms[k].os == crequest.device.os) && (adxtemp.adms[k].type == c_bid.type))
					{
						string adm = adxtemp.adms[k].adm;

						if (adm != "")
						{
							if (curl != "")
							{
								replaceMacro(adm, "#CURL#", curl.c_str());
							}

							if (cturl != "")//redirect为1时，cturl不替换"#CTURL#"
							{
								replaceMacro(adm, "#CTURL#", cturl.c_str());
							}

							if (c_bid.sourceurl != "")
							{
								replaceMacro(adm, "#SOURCEURL#", c_bid.sourceurl.c_str());
							}

							if (c_bid.monitorcode != "")
							{
								string moncode = c_bid.monitorcode;

								replaceMacro(adm, "#MONITORCODE#", moncode.c_str());
							}

							if (c_bid.cmonitorurl.size() > 0)
							{
								replaceMacro(adm, "#CMONITORURL#", c_bid.cmonitorurl[0].c_str());
							}

							if (iurl != "")
							{
								replaceMacro(adm, "#IURL#", iurl.c_str());
							}

							bid.adm = adm;
						}

						break;
					}
				}
			}
			else if (crequest.imp[0].type == IMP_NATIVE)
			{
				if (iurl != "")
					bid.admobject.native.imptrackers.push_back(iurl);
				for (int k = 0; k < c_bid.imonitorurl.size(); ++k)
				{
					bid.admobject.native.imptrackers.push_back(c_bid.imonitorurl[k]);
				}
				bid.admobject.native.link.url = curl;
				if (c_bid.redirect != 1)
					bid.admobject.native.link.clicktrackers.push_back(cturl);
				for (int k = 0; k < c_bid.cmonitorurl.size(); ++k)
				{
					bid.admobject.native.link.clicktrackers.push_back(c_bid.cmonitorurl[k]);
				}

				// assets
				for (uint32_t k = 0; k < c_bid.native.assets.size(); ++k)
				{
					COM_ASSET_RESPONSE_OBJECT &asset = c_bid.native.assets[k];
					ASSETRESPONSE asset_response;
					asset_response.id = asset.id;
					asset_response.required = asset.required;
					asset_response.type = asset.type;
					if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						asset_response.title.text = asset.title.text;
					}
					else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						asset_response.img.url = asset.img.url;
						asset_response.img.w = asset.img.w;
						asset_response.img.h = asset.img.h;
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						asset_response.data.value = asset.data.value;
						asset_response.data.label = asset.data.label;
					}
					bid.admobject.native.assets.push_back(asset_response);
				}
			}

			bid.adomain.push_back(c_bid.adomain);

			//inmobi said : "bid.iurl is used only in our audit tool"
			// 			if (adxtemp.iurl.size() > 0) 
			// 			{
			// 				bid.iurl = adxtemp.iurl[0];
			// 				
			// 			}	

			bid.cid = c_bid.cid;
			bid.crid = c_bid.mapid;

			set<uint8_t>::iterator it = c_bid.attr.begin();
			for (; it != c_bid.attr.end(); it++)
			{
				bid.attr.push_back(*it);
			}
			seatbid.bid.push_back(bid);
		}
		m.seatbid.push_back(seatbid);
	}

	m.bidid = c.bidid;

	m.cur = "CNY";
}

static bool callback_process_crequest(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = adxtemplate.ratio;

	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
		return false;

	for (int i = 0; i < crequest.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];

		if (cimp.bidfloorcur != "CNY")
		{
			return false;
		}

		cflog(g_logid_local, LOGINFO, "process_crequest_callback, bidfloor_adx[%s]:%f",
			cimp.bidfloorcur.c_str(), cimp.bidfloor);
	}

	return true;
}

static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	//	double ratio = adxtemplate.ratio;

	if ((ratio > -0.000001) && (ratio < 0.000001))
	{
		cflog(g_logid_local, LOGERROR, "callback_check_price failed! ratio:%f", ratio);

		return false;
	}

	for (int i = 0; i < crequest.imp.size(); i++)
	{
		double price_gap_usd = (price - crequest.imp[i].bidfloor);

		if (price_gap_usd < 0.01)//1 USD cent
		{
			cflog(g_logid_local, LOGERROR, "callback_check_price failed! bidfloor[cny]:%f, cbid.price[cny]:%f, ratio:%f, price_gap_usd[usd]:%f",
				crequest.imp[i].bidfloor, price, ratio, price_gap_usd);

			return false;
		}
		else
		{
			cflog(g_logid_local, LOGINFO, "callback_check_price success! bidfloor[cny]:%f, cbid.price[cny]:%f, ratio:%f, price_gap_usd[usd]:%f",
				crequest.imp[i].bidfloor, price, ratio, price_gap_usd);
		}
	}

	return true;
}

static bool callback_check_exts(IN const COM_REQUEST &crequest, IN const map<uint8_t, string> &exts)
{
	return true;
}

int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata,
	OUT string &senddata, OUT string &requestid, OUT string &response_short, OUT FLOW_BID_INFO &flow_bid)
{
	int errcode = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	MESSAGEREQUEST mrequest;//adx msg request
	COM_REQUEST crequest;//common msg request
	MESSAGERESPONSE mresponse;
	COM_RESPONSE cresponse;
	ADXTEMPLATE adxtemp;
	string output = "";
	string str_ferrcode;
	int imptype = IMP_UNKNOWN;
	timespec ts1, ts2;
	long lasttime = 0;
	getTime(&ts1);

	if ((recvdata == NULL) || (ctx == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata or ctx is null");
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		cflog(g_logid_local, LOGERROR, "recvdata->data is null or recvdata->length is 0");
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	init_message_request(mrequest);
	if (!parse_inmobi_request(recvdata->data, mrequest, imptype))
	{
		cflog(g_logid_local, LOGERROR, "parse_inmobi_request failed! Mark errcode: E_BAD_REQUEST");
		errcode = E_BAD_REQUEST;
		goto exit;
	}

	requestid = mrequest.id;

	init_com_message_request(&crequest);
	errcode = transform_request(mrequest, imptype, crequest);

	if (errcode != E_SUCCESS)
	{
		cflog(g_logid_local, LOGERROR, "transform_request failed!");
		goto exit;
	}
	flow_bid.set_req(crequest);

	if (crequest.id == "")
	{
		errcode = E_REQUEST_NO_BID;
		goto exit;
	}

	if (imptype == IMP_UNKNOWN)
	{
		errcode = E_REQUEST_IMP_TYPE;
		goto exit;
	}

	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!",
			crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		errcode = E_REQUEST_DEVICE_IP;
		goto exit;
	}

	init_com_message_response(&cresponse);
	init_com_template_object(&adxtemp);
	ferrcode = bid_filter_response(ctx, index, crequest, 
		callback_process_crequest, NULL, callback_check_price, NULL, 
		&cresponse, &adxtemp);
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	flow_bid.set_bid_flow(ferrcode);
	errcode = ferrcode.errcode;
	g_ser_log.send_bid_log(index, crequest, cresponse, errcode);

exit:
	if (errcode == E_SUCCESS)
	{
		init_message_response(mresponse);
		transform_response(cresponse, adxtemp, crequest, mresponse);

		setInmobiJsonResponse(mresponse, imptype, output);

		senddata = string("Content-Type: application/json\r\nx-openrtb-version: 2.0\r\nContent-Length: ")
			+ intToString(output.size()) + "\r\n\r\n" + output;

		response_short =
			cresponse.bidid + string(",") +
			cresponse.seatbid[0].bid[0].mapid + string(",") +
			crequest.device.deviceid + string(",") +
			intToString(crequest.device.deviceidtype) + string(",") +
			doubleToString(cresponse.seatbid[0].bid[0].price);
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());
	va_cout("%s,spent time:%ld,err:0x%x,desc:%s", crequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());

	flow_bid.set_err(errcode);
	return errcode;
}
