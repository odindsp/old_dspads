#include<stdio.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "bid_filter_type.h"

/* response init function */
void init_com_bid_object(COM_BIDOBJECT *pcbid)
{
	COM_BIDOBJECT *c = pcbid;

//	c->mapid = c->impid = c->adid = c->groupid =c->cbundle = c->adomain = c->curl
//		= c->monitorcode = c->sourceurl = c->cid = c->crid = "";

	c->type = c->ftype = c->ctype = c->w = c->h = 0;
}

void init_com_message_response(COM_RESPONSE *pcresponse)
{
	COM_RESPONSE *c = pcresponse;
//	c->id = c->bidid = c->cur = "";
}

/* request init function */
static void init_com_banner_object(COM_BANNEROBJECT &banner)
{
	banner.w = banner.h = 0;
}

static void init_com_video_object(COM_VIDEOOBJECT &video)
{
	video.display = video.minduration = video.maxduration = video.w = video.h = 0;
	video.is_limit_size = true;
}

void init_com_asset_request_object(COM_ASSET_REQUEST_OBJECT &asset)
{
	asset.id = asset.required = 0;

	asset.type = NATIVE_ASSETTYPE_UNKNOWN; 

	asset.title.len = 
		asset.img.type = asset.img.w = asset.img.wmin = asset.img.h = asset.img.hmin = 
		asset.data.type = asset.data.len = 0;
}

void init_com_asset_response_object(COM_ASSET_RESPONSE_OBJECT &asset)
{
	asset.id = asset.required = 0;

	asset.type = NATIVE_ASSETTYPE_UNKNOWN;

	asset.img.w = asset.img.h = 0;

	asset.img.type = asset.data.type = 0;
}

void init_com_native_request_object(COM_NATIVE_REQUEST_OBJECT &native)
{
	native.layout = NATIVE_LAYOUTTYPE_UNKNOWN;
	native.plcmtcnt = 1;//at least one.
	native.img_num = 0;
	native.bctype.clear();
	native.assets.clear();
}

void init_com_banner_request_object(COM_BANNEROBJECT &banner)
{
	banner.w = 0;
	banner.h = 0;
	banner.battr.clear();
	banner.btype.clear();
}

void init_com_native_response_object(COM_NATIVE_RESPONSE_OBJECT &native)
{
}

static void init_com_impression_ext(COM_IMPEXT &ext)
{
	ext.instl = INSTL_NO;
	ext.splash = SPLASH_NO;
	ext.data = "";
	ext.advid = 0;
}

void init_com_impression_object(COM_IMPRESSIONOBJECT *pcimp)
{
	COM_IMPRESSIONOBJECT *c = pcimp;

	//	c->id = "";

	c->type = IMP_UNKNOWN;
	init_com_banner_object(c->banner);
	init_com_video_object(c->video);
	init_com_native_request_object(c->native);

	c->requestidlife = REQUESTIDLIFE;
	c->bidfloor = 0.00;
	c->adpos = ADPOS_UNKNOWN;
	init_com_impression_ext(c->ext);
}

static void init_com_app_object(COM_APPOBJECT &app)
{
//	app.id = app.name = app.bundle = app.storeurl ="";
}

static void init_com_geo_object(COM_GEOOBJECT &geo)
{
	geo.lat = geo.lon = 0;
}

static void init_com_device_object(COM_DEVICEOBJECT &device)
{
//	device.deviceid = device.did = device.dpid = device.ua = device.ip = device.make
//		= device.model = device.osv = "";

	device.deviceidtype = device.carrier = device.os
		= device.connectiontype = device.devicetype = device.location= 0;

	device.make = DEFAULT_MAKE;
	memset(device.locid, 0, DEFAULTLENGTH + 1);

	init_com_geo_object(device.geo);
}

static void init_com_user_object(COM_USEROBJECT &user)
{
   user.gender = GENDER_UNKNOWN;
   user.yob = -1;
   init_com_geo_object(user.geo);
}

void init_com_message_request(COM_REQUEST *pcrequest)
{
	COM_REQUEST *c = pcrequest;
	c->at = BID_RTB;
//	c->id = "";
	init_com_app_object(c->app);
	init_com_device_object(c->device);
	init_com_user_object(c->user);
	pcrequest->is_recommend = 0;
	pcrequest->is_secure = 0;
	pcrequest->support_deep_link = 0;
}

void init_com_template_object(ADXTEMPLATE *padxtemp)
{
	padxtemp->adx = 0;
//	padxtemp->aurl = padxtemp->nurl = "";
}

static void print_request_native_asset(COM_ASSET_REQUEST_OBJECT &asset)
{
	cout<<"asset id="<<asset.id<<endl;
	cout<<"required="<<(int)asset.required<<endl;
	cout<<"asset type="<<(int)asset.type<<endl;
	if (asset.type == NATIVE_ASSETTYPE_TITLE)
	{
		cout<<"title len="<<asset.title.len<<endl;
	}
	else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
	{
		COM_IMAGE_REQUEST_OBJECT &img = asset.img;
		cout<<"img type="<<(int)img.type<<endl;	
		cout<<"w="<<(int)img.w<<endl;
		cout<<"wmin="<<(int)img.wmin<<endl;
		cout<<"h="<<(int)img.h<<endl;
		cout<<"hmin="<<(int)img.hmin<<endl;
		set<uint8_t>::iterator p = img.mimes.begin();
		while (p != img.mimes.end())
		{
			cout<<"mimes="<<(int)*p<<endl;
			++p;	
		}
	}
	else if (asset.type == NATIVE_ASSETTYPE_DATA)
	{
		cout<<"data type="<<(int)asset.data.type<<endl;	
		cout<<"data len="<<(int)asset.data.len<<endl;
	}
}

void print_com_request(IN COM_REQUEST &commrequest)
{
#ifndef DEBUG
	return;
#endif // DEBUG
	cout<<"-----common request-----"<<endl;
	cout<<"id="<<commrequest.id<<endl;
	cout<<"at="<<(int)commrequest.at<<endl;
	cout<<"impression:--------"<<endl;
	for(uint32_t i = 0; i < commrequest.imp.size(); ++i)
	{
		cout<<"id="<<commrequest.imp[i].id<<endl;
		cout<<"imptype="<<(int)commrequest.imp[i].type<<endl;
		if(commrequest.imp[i].type == IMP_BANNER) // bannner
		{
			COM_BANNEROBJECT &banner = commrequest.imp[i].banner;
			cout<<"w="<<banner.w<<endl;
			cout<<"h="<<banner.h<<endl;
			set<uint8_t>::iterator p = banner.btype.begin();
			while(p != banner.btype.end())
			{
				cout<<"btype="<<(int)*p<<endl;
				++p;
			}
		}
		else if(commrequest.imp[i].type == IMP_VIDEO)
		{
			COM_VIDEOOBJECT &video = commrequest.imp[i].video;
			cout <<"mimes :"<<endl;
			set<uint8_t>::iterator it;
			for(it=video.mimes.begin();it!=video.mimes.end();it++) 
				cout<<(int)*it <<endl;   
			cout<<"video display="<<(int)video.display<<endl;
			cout<<"video mindur="<<video.minduration<<endl;
			cout<<"video maxdur="<<video.maxduration<<endl;
			cout<<"video w="<<video.w<<endl;
			cout<<"video h="<<video.h<<endl;
			cout<<"video is_limit_size=" <<video.is_limit_size <<endl;
		}
		else if(commrequest.imp[i].type == IMP_NATIVE)
		{
			COM_NATIVE_REQUEST_OBJECT &native = commrequest.imp[i].native;
			cout<<"layout="<<(int)native.layout<<endl;
			cout<<"plcmtcnt="<<(int)native.plcmtcnt<<endl;
			set<uint8_t>::iterator p = native.bctype.begin();
			while(p != native.bctype.end())
			{
				cout<<"bctype="<<(int)*p<<endl;
				++p;
			}

			for(uint32_t j = 0; j < native.assets.size(); ++j)
			{
				print_request_native_asset(native.assets[j]);	
			}
		}

		cout<<"bidfloor="<<commrequest.imp[i].bidfloor<<endl;
		cout<<"adpos="<<(int)commrequest.imp[i].adpos<<endl;
		COM_IMPEXT &ext = commrequest.imp[i].ext;
		cout<<"instl="<<(int)ext.instl<<endl;
		cout<<"splash="<<(int)ext.splash<<endl;
	}

	cout<<"app:--------"<<endl;
	COM_APPOBJECT &app = commrequest.app;
	cout<<"id="<<app.id<<endl;
	cout<<"name="<<app.name<<endl;
	cout<<"bundle="<<app.bundle<<endl;
	cout<<"storeurl="<<app.storeurl<<endl;
	for(set<uint32_t>::iterator pcat = app.cat.begin(); pcat != app.cat.end(); ++pcat)
	{
		printf("app cat=0x%x\n",*pcat);
	}

	cout<<"device:--------"<<endl;
	COM_DEVICEOBJECT &device = commrequest.device;
	map<uint8_t,string>::iterator dids = device.dids.begin();
	for(; dids != device.dids.end(); ++dids)
	{
		cout<<"did=" <<dids->second<<endl;
		cout<<"didtype="<<(int)dids->first<<endl;
	}

	dids = device.dpids.begin();
	for(; dids != device.dpids.end(); ++dids)
	{
		cout<<"dpid=" <<dids->second<<endl;
		cout<<"dpidtype="<<(int)dids->first<<endl;
	}
	cout<<"ua="<<device.ua<<endl;
	cout<<"ip="<<device.ip<<endl;
	cout<<"location="<<device.location<<endl;
	cout<<"lat="<<device.geo.lat<<endl;
	cout<<"lon="<<device.geo.lon<<endl;
	cout<<"carrier="<<(int)device.carrier<<endl;
	cout<<"make="<<(int)device.make<<endl;
	cout<<"original make="<<device.makestr<<endl;
	cout<<"model="<<device.model<<endl;
	cout<<"os="<<(int)device.os<<endl;
	cout<<"osv="<<device.osv<<endl;
	cout<<"connectiontype="<<(int)device.connectiontype<<endl;
	cout<<"devicetype="<<(int)device.devicetype<<endl;

	cout<<"user:--------"<<endl;
	COM_USEROBJECT &user = commrequest.user;
	cout<<"id="<<user.id<<endl;
	cout<<"gender="<<(int)user.gender<<endl;
	cout<<"yob="<<user.yob<<endl;
	cout<<"keywords="<<user.keywords<<endl;
	cout<<"searchkey="<<user.searchkey<<endl;
	cout<<"userlat="<<user.geo.lat<<endl;
	cout<<"userlon="<<user.geo.lon<<endl;

	set<uint32_t>::iterator p = commrequest.bcat.begin();
	while(p != commrequest.bcat.end())
	{
		printf("bcat=0x%x\n",*p);
		++p;
	}

	set<string>::iterator pbadv = commrequest.badv.begin();
	while(pbadv != commrequest.badv.end())
	{
		cout<<"badv="<<*pbadv<<endl;
		++pbadv;
	}

	cout<<"is_secure="<<commrequest.is_secure<<endl;
	return;
}

static void print_response_native_asset(COM_ASSET_RESPONSE_OBJECT &asset)
{
	cout<<"asset id="<<asset.id<<endl;
	cout<<"required="<<(int)asset.required<<endl;
	cout<<"asset type="<<(int)asset.type<<endl;
	if (asset.type == NATIVE_ASSETTYPE_TITLE)
	{
		cout<<"title text="<<asset.title.text<<endl;
	}
	else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
	{
		COM_IMAGE_RESPONSE_OBJECT &img = asset.img;
		cout<<"w="<<(int)img.w<<endl;
		cout<<"h="<<(int)img.h<<endl;
		cout<<"url="<<img.url<<endl;
	}
	else if (asset.type == NATIVE_ASSETTYPE_DATA)
	{
		cout<<"data label="<<asset.data.label<<endl;	
		cout<<"data value="<<asset.data.value<<endl;
	}
}

static void print_bid_object(COM_BIDOBJECT &bidobject)
{
	cout<<"bid:--------"<<endl;
	cout<<"mapid="<<bidobject.mapid<<endl;
	cout<<"impid="<<bidobject.impid<<endl;
	cout<<"price="<<bidobject.price<<endl;
	cout<<"adid="<<bidobject.adid<<endl;
	cout<<"groupid="<<bidobject.groupid<<endl;
	set<uint32_t>::iterator p = bidobject.cat.begin();
	for(p; p != bidobject.cat.end(); ++p)
	{
		cout<<"cat="<<(int)*p<<endl;
	}
	cout<<"type="<<(int)bidobject.type<<endl;
	cout<<"ftype="<<(int)bidobject.ftype<<endl;
	cout<<"ctype="<<(int)bidobject.ctype<<endl;
	cout<<"bundle="<<bidobject.bundle<<endl;
	cout<<"apkname="<<bidobject.apkname<<endl;
	cout<<"adomain="<<bidobject.adomain<<endl;
	cout<<"w="<<bidobject.w<<endl;
	cout<<"h="<<bidobject.h<<endl;
	cout<<"curl="<<bidobject.curl<<endl;
	cout<<"redirect="<<(int)bidobject.redirect<<endl;
	cout<<"effectmonitor="<<(int)bidobject.effectmonitor<<endl;
	cout<<"monitorcode="<<bidobject.monitorcode<<endl;
	for(uint32_t i = 0; i < bidobject.cmonitorurl.size(); ++i)
		cout<<"cmonitorurl="<<bidobject.cmonitorurl[i]<<endl;
	for(uint32_t i = 0; i<bidobject.imonitorurl.size(); ++i)
		cout<<"imonitorurl="<<bidobject.imonitorurl[i]<<endl;
	cout<<"sourceurl=" <<bidobject.sourceurl<<endl;
	cout<<"cid=" <<bidobject.cid<<endl;
	cout<<"crid="<<bidobject.crid<<endl;

	GROUPINFO_EXT &group_ext = bidobject.groupinfo_ext;
	cout<<"advid="<<group_ext.advid<<endl;

	CREATIVE_EXT &cr_ext = bidobject.creative_ext;
	cout<<"cre_id="<<cr_ext.id<<endl;
	cout<<"cre_ext="<<cr_ext.ext<<endl;

	if (bidobject.type == ADTYPE_FEEDS)
	{
		COM_NATIVE_RESPONSE_OBJECT &native = bidobject.native;
		for (uint32_t i = 0; i < native.assets.size(); ++i)
		{
			print_response_native_asset(native.assets[i]);
		}
	}
	return;
}

void print_com_response(IN COM_RESPONSE &commresponse)
{
#ifndef DEBUG
	return;
#endif // DEBUG

	cout<<"id="<<commresponse.id<<endl;
	cout<<"bidid="<<commresponse.bidid<<endl;
	cout<<"cur="<<commresponse.cur<<endl;
	for(uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = commresponse.seatbid[i];
		for(uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT &bidobject = seatbidobject.bid[j];
			print_bid_object(bidobject);
		}
	}

	return;
}
