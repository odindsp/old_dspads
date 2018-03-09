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
#include "../../common/url_endecode.h"
#include "iflytek_response.h"

extern uint64_t g_logid_local, g_logid_local_traffic, g_logid, g_logid_response;
extern map<string, vector<uint32_t> > app_cat_table_adx2com;
extern map<string, vector<uint32_t> > adv_cat_table_adx2com;
//extern map<uint32_t, vector<string> > adv_cat_table_com2adx;
extern map<string, uint16_t> dev_make_table;
extern MD5_CTX hash_ctx;

static void init_geo_object(GEOOBJECT &geo)//
{
	geo.lat = geo.lon = 0;
}

static void init_user_object(USEROBJECT &user)//
{
	user.yob = 0;
}

static void init_app_object(APPOBJECT &app)//
{
}

static void init_device_ext(DEVICEEXT &ext)//
{
	ext.sw = ext.sh = ext.brk = 0;
}
static void init_device_object(DEVICEOBJECT &device)//
{
	device.js = device.connectiontype = device.devicetype = 0;
	init_device_ext(device.ext);
	init_geo_object(device.geo);
}

static void init_title_object(TITLEOBJECT &title)
{
	title.len = 0;
}

static void init_desc_object(DESCOBJECT &desc)
{
	desc.len = 0;
}

static void init_icon_object(ICONOBJECT &icon)
{
	icon.w = icon.h = icon.wmin = icon.hmin = 0;
}

static void init_img_object(IMGOBJECT &img)
{
	img.w = img.h = img.wmin = img.hmin = 0;
}

static void init_native_request(NATIVEREQUEST &request)
{
	init_title_object(request.title);

	init_desc_object(request.desc);

	init_icon_object(request.icon);

	init_img_object(request.img);
}
static void init_native_ext(NATIVEEXT &ext)
{
}
static void init_native_object(NATIVEOBJECT &native)
{
	native.battr = 0;
	
	init_native_request(native.natreq);

	init_native_ext(native.ext);
}

static void init_video_ext(VIDEOEXT &ext)
{
}
static void init_video_object(VIDEOOBJECT &video)
{
	video.protocol = video.w = video.h = video.minduration = video.maxduration = video.linearity = 0;

	init_video_ext(video.ext);
}

static void init_banner_ext(BANNEREXT &ext)
{
}
static void init_banner_object(BANNEROBJECT &banner)
{
	banner.w = banner.h = 0;

	init_banner_ext(banner.ext);
}

static void init_impression_object(IMPRESSIONOBJECT &imp)
{
	imp.instl = imp.bidfloor = 0;

	init_banner_object(imp.banner);

	init_video_object(imp.video);

	init_native_object(imp.native);
}

static void init_message_request(MESSAGEREQUEST &mrequest)//
{

	init_user_object(mrequest.user);

	init_app_object(mrequest.app);

	init_device_object(mrequest.device);

	mrequest.at = 1;

	mrequest.tmax = 0;
}

static void init_seat_bid(SEATBIDOBJECT &seatbid)
{
}

static void init_bid_object(BIDOBJECT &bid)
{
	bid.lattr = bid.w = bid.h = 0;
	bid.video.duration = 0;
}

static void init_message_response(MESSAGERESPONSE &response)
{
}

static void revert_json(string &text)
{
	replaceMacro(text, "\\", "");
}

static void serial_json(string macro, string data, string &text)
{
	int locate = 0;
	while(1)
	{

		locate = text.find(macro.c_str(), locate);
		if (locate != string::npos)
		{
			text.replace(locate, macro.size(), data.c_str());
			locate += data.size();
		}
		else
			break;
	}
	return;
}

static bool parse_inmobi_request(RECVDATA *recvdata, MESSAGEREQUEST &request)
{
	json_t *root = NULL;
	json_t *label = NULL;

	if(recvdata->length == 0)
		return false;

	json_parse_document(&root, recvdata->data);
	if(root == NULL)
		return false;

	if((label = json_find_first_label(root, "id")) != NULL)
		request.id = label->child->text;

	json_free_value(&root);

	return true;
}

int get_bid_response(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &senddata)
{
	int errcode = E_SUCCESS;
	MESSAGEREQUEST mrequest;//adx msg request

	if ((recvdata == NULL))
	{
//		cflog(g_logid_local, LOGERROR, "recvdata or ctx is null");
		errcode = E_INVALID_PARAM;
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
//		cflog(g_logid_local, LOGERROR, "recvdata->data is null or recvdata->length is 0");
		errcode = E_INVALID_PARAM;
		goto exit;
	}

	init_message_request(mrequest);
	if (!parse_inmobi_request(recvdata, mrequest))
	{
//		cflog(g_logid_local, LOGERROR, "parse_inmobi_request failed! Mark errcode: E_BAD_REQUEST");
		errcode = E_BAD_REQUEST;
		goto exit;
	}
	
  if (mrequest.id == "")
	{
		errcode = E_BAD_REQUEST;
		goto exit;
	}

exit:
  if (errcode == E_SUCCESS) 
  {
	  senddata = "Status: 204 OK\r\n\r\n";
  }

	return errcode;
}




