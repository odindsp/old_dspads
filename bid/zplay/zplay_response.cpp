#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <google/protobuf/text_format.h>
#include "../../common/json_util.h"
#include "../../common/url_endecode.h"
#include "../../common/bid_filter_dump.h"
#include "zplay_response.h"


extern map<string, vector<uint32_t> > zplay_app_cat_table;
extern map<string, uint16_t> dev_make_table;
extern uint64_t g_logid, g_logid_local, g_logid_response;
vector<string> AdvertisingType;
extern MD5_CTX hash_ctx;
extern uint64_t geodb;
extern int *numcount;

bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if (price >= crequest.imp[0].bidfloor)
		return true;
	else
		return false;
}

void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);
	uint32_t com_cat;

	sscanf(val.c_str(), "%x", &com_cat);//0x%x
	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}


static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &exts)
{
	bool filter = true;
	json_t *root = NULL, *label = NULL;

exit:
	return filter;
}

static bool callback_process_crequest(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = adxtemplate.ratio;

	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
	{
		cout << "callback process crequest failed:" << adxtemplate.adx << endl;
		return false;
	}

	for (int i = 0; i < crequest.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];
		cimp.bidfloor /= ratio;
	}
	return true;
}

static void initialize_native(IN const com::google::openrtb::NativeRequest &zplay_native, OUT COM_NATIVE_REQUEST_OBJECT &com_native)
{
	com_native.layout = zplay_native.layout();
	com_native.plcmtcnt = 1;
	for (int i = 0; i < zplay_native.assets().size(); ++i)
	{
		const com::google::openrtb::NativeRequest_Asset &zplay_asset = zplay_native.assets(i);
		COM_ASSET_REQUEST_OBJECT com_asset;
		init_com_asset_request_object(com_asset);
		com_asset.id = zplay_asset.id();
		com_asset.required = zplay_asset.required();
		if (zplay_asset.has_title())
		{
			com_asset.type = NATIVE_ASSETTYPE_TITLE;
			com_asset.title.len = zplay_asset.title().len() * 3;
		}
		else if (zplay_asset.has_img())
		{
			com_asset.type = NATIVE_ASSETTYPE_IMAGE;
			com_asset.img.type = zplay_asset.img().type();
			com_asset.img.w = zplay_asset.img().w();
			com_asset.img.h = zplay_asset.img().h();
		}
		else if (zplay_asset.has_data())
		{
			com_asset.type = NATIVE_ASSETTYPE_DATA;
			com_asset.data.type = zplay_asset.data().type();
			com_asset.data.len = zplay_asset.data().len() * 3;
		}

		com_native.assets.push_back(com_asset);
	}
}

static int TranferToCommonRequest(IN com::google::openrtb::BidRequest &zplay_request, OUT COM_REQUEST &crequest)
{
	int errorcode = E_SUCCESS;

	if (zplay_request.id() != "")
	{
		crequest.id = zplay_request.id();
	}
	else
	{
		errorcode = E_REQUEST_NO_BID;
		goto exit;
	}

	for (int i = 0; i < zplay_request.bcat().size(); i++)
	{
		vector<uint32_t> &v_bcat = zplay_app_cat_table[zplay_request.bcat(i)];

		for (int j = 0; j < v_bcat.size(); j++)
		{
			crequest.bcat.insert(v_bcat[j]);
		}
	}

	crequest.is_secure = 0;
	if (zplay_request.GetExtension(zadx::need_https))
	{
		crequest.is_secure = 1;
	}

	if (zplay_request.has_app())
	{
		crequest.app.id = zplay_request.app().id();
		crequest.app.name = zplay_request.app().name();
		crequest.app.bundle = zplay_request.app().bundle();
		crequest.app.storeurl = zplay_request.app().storeurl();

		for (int i = 0; i < zplay_request.app().cat().size(); i++)
		{
			vector<uint32_t> &v_cat = zplay_app_cat_table[zplay_request.app().cat(i)];

			for (int j = 0; j < v_cat.size(); j++)
			{
				crequest.app.cat.insert(v_cat[j]);
			}
		}
	}
	else
	{
		errorcode = E_REQUEST_APP;
		goto exit;
	}

	if (zplay_request.has_user())
	{
		crequest.user.id = zplay_request.user().id();
		crequest.user.yob = zplay_request.user().yob();
		string gender = zplay_request.user().gender();
		if (gender == "M")
			crequest.user.gender = GENDER_MALE;
		else if (gender == "F")
			crequest.user.gender = GENDER_FEMALE;
		else
			crequest.user.gender = GENDER_UNKNOWN;

		if (zplay_request.user().has_geo())
		{
			crequest.user.geo.lat = zplay_request.user().geo().lat();
			crequest.user.geo.lon = zplay_request.user().geo().lon();
		}
	}

	if (zplay_request.has_device())
	{
		writeLog(g_logid_local, LOGERROR, "zplayrequest.device.os:%s", zplay_request.device().os().c_str());
		if (!strcasecmp(zplay_request.device().os().c_str(), "Android"))
			crequest.device.os = OS_ANDROID;
		else if (!strcasecmp(zplay_request.device().os().c_str(), "iOS"))
			crequest.device.os = OS_IOS;
		else if (!strcasecmp(zplay_request.device().os().c_str(), "WP"))
			crequest.device.os = OS_WINDOWS;
		else crequest.device.os = OS_UNKNOWN;


		crequest.device.osv = zplay_request.device().osv();
		crequest.device.makestr = zplay_request.device().make();
		crequest.device.model = zplay_request.device().model();
		crequest.device.ip = zplay_request.device().ip();
		crequest.device.ua = zplay_request.device().ua();

		if (crequest.device.makestr != "")
		{
			map<string, uint16_t>::iterator it;

			for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
			{
				if (crequest.device.makestr.find(it->first) != string::npos)
				{
					crequest.device.make = it->second;
					break;
				}
			}
		}

		if (crequest.device.os == OS_ANDROID)
		{
			if (zplay_request.device().macsha1() != "")
			{
				crequest.device.dids.insert(make_pair(DID_MAC_SHA1, zplay_request.device().macsha1()));
				writeLog(g_logid_local, LOGERROR, "zplayrequest.device.macsha1:%s", zplay_request.device().macsha1().c_str());
			}

			if (zplay_request.device().didsha1() != "")
			{
				crequest.device.dids.insert(make_pair(DID_IMEI_SHA1, zplay_request.device().didsha1()));
				writeLog(g_logid_local, LOGERROR, "zplayrequest.device.didsha1:%s", zplay_request.device().didsha1().c_str());
			}

			if (zplay_request.device().dpidsha1() != "")
				crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, zplay_request.device().dpidsha1()));

		}
		if (crequest.device.os == OS_IOS)
		{
			if (zplay_request.device().dpidsha1() != "")
				crequest.device.dpids.insert(make_pair(DPID_IDFA_SHA1, zplay_request.device().dpidsha1()));

		}

		switch (zplay_request.device().connectiontype())
		{
		case 0:
			crequest.device.connectiontype = CONNECTION_UNKNOWN;
			break;
		case 2:
			crequest.device.connectiontype = CONNECTION_WIFI;
			break;
		case 3:
			crequest.device.connectiontype = CONNECTION_CELLULAR_UNKNOWN;
			break;
		case 4:
			crequest.device.connectiontype = CONNECTION_CELLULAR_2G;
			break;
		case 5:
			crequest.device.connectiontype = CONNECTION_CELLULAR_3G;
			break;
		case 6:
			crequest.device.connectiontype = CONNECTION_CELLULAR_4G;
			break;
		default:
			crequest.device.connectiontype = CONNECTION_UNKNOWN;
			break;

		}

		switch (zplay_request.device().devicetype())
		{
		case 1:
		case 4:
			crequest.device.devicetype = DEVICE_PHONE;
			break;
		case 5:
			crequest.device.deviceidtype = DEVICE_TABLET;
			break;
		default:
			crequest.device.devicetype = DEVICE_UNKNOWN;
			break;
		}

		if (zplay_request.device().has_geo())
		{
			crequest.device.geo.lat = zplay_request.device().geo().lat();
			crequest.device.geo.lon = zplay_request.device().geo().lon();
		}

		if (zplay_request.device().GetExtension(zadx::plmn) != "")
		{
			if (zplay_request.device().GetExtension(zadx::plmn) == "46000")
				crequest.device.carrier = CARRIER_CHINAMOBILE;
			else if (zplay_request.device().GetExtension(zadx::plmn) == "46001")
				crequest.device.carrier = CARRIER_CHINAUNICOM;
			else if (zplay_request.device().GetExtension(zadx::plmn) == "46002")
				crequest.device.carrier = CARRIER_CHINATELECOM;
			else
				crequest.device.carrier = CARRIER_UNKNOWN;
		}

		if (zplay_request.device().GetExtension(zadx::imei) != "")
			crequest.device.dids.insert(make_pair(DID_IMEI, zplay_request.device().GetExtension(zadx::imei)));

		if (zplay_request.device().GetExtension(zadx::mac) != "")
			crequest.device.dids.insert(make_pair(DID_MAC, zplay_request.device().GetExtension(zadx::mac)));

		if (zplay_request.device().GetExtension(zadx::android_id) != "")
			crequest.device.dpids.insert(make_pair(DPID_ANDROIDID, zplay_request.device().GetExtension(zadx::android_id)));
	}
	else
	{
		errorcode = E_REQUEST_DEVICE;
		goto exit;
	}

	if (zplay_request.imp_size() > 0)
	{
		for (int i = 0; i < zplay_request.imp_size(); ++i)
		{
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);

			const ::com::google::openrtb::BidRequest_Imp &zplay_imp = zplay_request.imp(i);

			imp.id = zplay_imp.id();
			imp.bidfloor = zplay_imp.bidfloor() / 100; //转化为元/CPM
			imp.bidfloorcur = "CNY";
			imp.requestidlife = 3600;
			int adpos = 0;

			if (zplay_imp.has_native())
			{
				if (zplay_imp.native().has_request_native())
				{
					imp.type = IMP_NATIVE;
					writeLog(g_logid_local, LOGDEBUG, "id=%s,pid=%s", crequest.id.c_str(), zplay_imp.tagid().c_str());
					initialize_native(zplay_imp.native().request_native(), imp.native);
				}
				else
				{
					errorcode = E_REQUEST_NATIVE;
					goto exit;
				}
			}
			/*else if(zplay_imp.has_video())
			{
			imp.type = IMP_VIDEO;
			imp.video.minduration = zplay_imp.video().minduration();
			imp.video.maxduration = zplay_imp.video().maxduration();
			imp.video.w = zplay_imp.video().w();
			imp.video.h = zplay_imp.video().h();
			adpos = zplay_imp.video().pos();
			}*/
			else if (zplay_imp.has_banner())
			{
				imp.type = IMP_BANNER;
				imp.banner.w = zplay_imp.banner().w();
				imp.banner.h = zplay_imp.banner().h();
				imp.ext.instl = zplay_imp.instl();
				imp.ext.splash = zplay_imp.GetExtension(zadx::is_splash_screen);
				adpos = zplay_imp.banner().pos();
			}
			else
			{
				errorcode = E_REQUEST_IMP_TYPE;
				goto exit;
			}

			switch (adpos)
			{
			case 1:
				imp.adpos = ADPOS_FIRST;
				break;
			case 3:
				imp.adpos = ADPOS_SECOND;
				break;
			case 4:
				imp.adpos = ADPOS_HEADER;
				break;
			case 5:
				imp.adpos = ADPOS_FOOTER;
				break;
			case 6:
				imp.adpos = ADPOS_SIDEBAR;
				break;
			case 7:
				imp.adpos = ADPOS_FULL;
				break;
			default:
				imp.adpos = ADPOS_UNKNOWN;
				break;
			}

			crequest.imp.push_back(imp);
		}
	}
	else
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		goto exit;
	}

	if (crequest.device.ip != "")
	{
		crequest.device.location = getRegionCode(geodb, crequest.device.ip);;
	}

	writeLog(g_logid_local, LOGERROR, "TransferToCommonRequest  end...");

exit:
	return errorcode;
}

bool TranferToZplayResponse(COM_REQUEST comrequest, COM_RESPONSE &comresponse, ADXTEMPLATE *adxinfo, 
	com::google::openrtb::BidResponse &zplay_response/*,
							IN com::google::openrtb::BidRequest &zplay_request*/)
{
	bool errorcode = true;
	string trace_url_par = string("&") + get_trace_url_par(comrequest, comresponse);
	zplay_response.set_id(comresponse.id);
	for (int i = 0; i < comresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT *seatbid = &comresponse.seatbid[i];
		com::google::openrtb::BidResponse_SeatBid *zplay_seadbid = zplay_response.add_seatbid();
		for (int j = 0; j < seatbid->bid.size(); ++j)
		{
			double price = 0.0;
			COM_BIDOBJECT *bid = &seatbid->bid[j];
			//replace_https_url(*adxinfo,comrequest.is_secure);

			com::google::openrtb::BidResponse_SeatBid_Bid *zplay_bid = zplay_seadbid->add_bid();

			zplay_bid->set_id(comresponse.id);
			zplay_bid->set_impid(bid->impid);
			price = (double)bid->price * 100;
			zplay_bid->set_price((uint32_t)price);
			zplay_bid->set_adid(bid->adid);

			if (bid->ctype == CTYPE_DOWNLOAD_APP || bid->type == CTYPE_DOWNLOAD_APP_FROM_APPSTORE)
				zplay_bid->set_bundle(bid->bundle);

			string curl = bid->curl;
			string cturl;

			for (int k = 0; k < adxinfo->cturl.size(); ++k)
			{
				cturl = adxinfo->cturl[k] + trace_url_par;
				if (bid->redirect)
				{
					int len = 0;
					char *en_curl = NULL;
					string curl_t = curl;
					en_curl = urlencode(curl_t.c_str(), curl_t.size(), &len);
					cturl += "&curl=" + string(en_curl, len);
					free(en_curl);
					curl = cturl;
					cturl = "";
				}
				break;
			}

			if (comrequest.imp[0].type == IMP_NATIVE || comrequest.imp[0].type == IMP_BANNER)
			{
				for (int k = 0; k < bid->imonitorurl.size(); ++k)
				{
					zplay_bid->AddExtension(zadx::imptrackers, bid->imonitorurl[k]);
				}

				for (int k = 0; k < adxinfo->iurl.size(); ++k)
				{
					string iurl = adxinfo->iurl[k]+ trace_url_par;
					zplay_bid->AddExtension(zadx::imptrackers, iurl);
				}

				for (int k = 0; k < bid->cmonitorurl.size(); ++k)
				{
					zplay_bid->AddExtension(zadx::clktrackers, bid->cmonitorurl[k]);
				}
				if (cturl != "")
					zplay_bid->AddExtension(zadx::clktrackers, cturl);
			}


			if (comrequest.imp[0].type == IMP_NATIVE)
			{
				com::google::openrtb::NativeResponse *native_response = zplay_bid->mutable_adm_native();
				//for(int k = 0; k < bid->imonitorurl.size(); ++k)
				//{
				//	native_response->add_imptrackers(bid->imonitorurl[k]);
				//}

				//for(int k = 0 ; k < adxinfo->iurl.size(); ++k)
				//{
				//	native_response->add_imptrackers(adxinfo->iurl[k]);
				//}

				com::google::openrtb::NativeResponse_Link *link = native_response->mutable_link();
				link->set_url(curl);
				int ctype = CTYPE_UNKNOWN;
				switch (bid->ctype)
				{
				case CTYPE_OPEN_WEBPAGE:
					ctype = 2;
					break;
				case CTYPE_DOWNLOAD_APP:
					ctype = 6;
					break;
				case CTYPE_DOWNLOAD_APP_FROM_APPSTORE:
					ctype = 6;
					break;
				case CTYPE_CALL:
					ctype = 4;
					break;
				case CTYPE_OPEN_WEBPAGE_WITH_WEBVIEW:
					ctype = 1;
					break;
				default:
					break;
				}
				link->SetExtension(zadx::link_type, ctype);
				//for(int k = 0; k < bid->cmonitorurl.size(); ++k)
				//{
				//	link->add_clicktrackers(bid->cmonitorurl[k]);
				//}
				//if (cturl != "")
				//	link->add_clicktrackers(cturl);

				for (int k = 0; k < bid->native.assets.size(); ++k)
				{
					COM_ASSET_RESPONSE_OBJECT *com_assets = &bid->native.assets[k];
					com::google::openrtb::NativeResponse_Asset *zplay_asset = native_response->add_assets();
					zplay_asset->set_id(com_assets->id);

					if (com_assets->type == NATIVE_ASSETTYPE_TITLE)
					{
						com::google::openrtb::NativeResponse_Asset_Title *zplay_title = zplay_asset->mutable_title();
						zplay_title->set_text(com_assets->title.text);
					}
					else if (com_assets->type == NATIVE_ASSETTYPE_IMAGE)
					{
						com::google::openrtb::NativeResponse_Asset_Image *zplay_image = zplay_asset->mutable_img();
						zplay_image->set_url(com_assets->img.url);
					}
					else if (com_assets->type == NATIVE_ASSETTYPE_DATA)
					{
						com::google::openrtb::NativeResponse_Asset_Data *zplay_data = zplay_asset->mutable_data();
						zplay_data->set_label(com_assets->data.label);
						zplay_data->set_value(com_assets->data.value);
					}
				}

			}
			else if (comrequest.imp[0].type == IMP_BANNER)
			{
				zplay_bid->set_iurl(bid->sourceurl);
				zplay_bid->SetExtension(zadx::clkurl, curl);
			}
			/*else if (comrequest.imp[0].type == IMP_VIDEO)
			{
			zplay_bid->set_iurl("http://test.pxene.com:59001/dav/65d106ff-2cb9-4ae7-a2d8-897fe3f05f64/image/68bbe20b73a0479e9967f8030ecee4aa.jpg");
			uint8_t os = comrequest.device.os;
			int creativetype = bid->type;
			string admurl;
			for (int k = 0; k < adxinfo->adms.size(); ++k)
			{
			if (adxinfo->adms[k].os == os && adxinfo->adms[k].type == creativetype)
			{
			admurl = adxinfo->adms[k].adm;
			break;
			}
			}

			if (admurl.length() > 0)
			{
			replaceMacro(admurl,"#SOURCEURL#",bid->sourceurl.c_str());
			replaceMacro(admurl,"#CURL#",curl.c_str());

			replaceMacro(admurl,"#CTURL#",cturl.c_str());
			string cmonitorurl;
			if (bid->cmonitorurl.size() > 0)
			{
			cmonitorurl = bid->cmonitorurl[0];
			}
			replaceMacro(admurl,"#CMONITORURL#",cmonitorurl.c_str());

			string iurl;
			if (adxinfo->iurl.size() > 0)
			{
			iurl = adxinfo->iurl[0];
			}
			replaceMacro(admurl,"#IURL#",iurl.c_str());

			string imonitorurl;
			if (bid->imonitorurl.size() > 0)
			{
			imonitorurl = bid->imonitorurl[0];
			}
			replaceMacro(admurl,"#IMONITORURL#",imonitorurl.c_str());
			zplay_bid->set_adm(admurl);
			}
			else
			{
			va_cout("zplay video adm is empty!");
			cflog(g_logid_local,LOGDEBUG, "zplay video adm is empty! id = %s,os = %d,type = %d",comrequest.id.c_str(),os,creativetype);
			}
			}*/
		}
	}

	return errorcode;
}

int zplayResponse(IN uint8_t index, IN uint64_t ctx, IN RECVDATA *arg, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int errorcode = E_SUCCESS;
	char *outputdata = NULL;
	RECVDATA *recvdata = arg;
	ADXTEMPLATE adxinfo;
	FULL_ERRCODE ferrcode;
	bool parsesuccess = false;
	string requestid = "";
	string str_ferrcode = "";

	COM_REQUEST comrequest;
	com::google::openrtb::BidRequest zplay_request;

	COM_RESPONSE comresponse;
	com::google::openrtb::BidResponse zplay_response;

	struct timespec ts1, ts2;
	long lasttime = 0;
	getTime(&ts1);

	if ((recvdata == NULL) || (ctx == 0))
	{
		writeLog(g_logid_local, LOGERROR, string("recvdata or ctx is NULL"));
		errorcode = E_BAD_REQUEST;
		return  errorcode;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		writeLog(g_logid_local, LOGERROR, string("recvdata->data is null or recvdata->length is 0"));
		errorcode = E_BAD_REQUEST;
		goto exit;
	}

	init_com_template_object(&adxinfo);
	init_com_message_request(&comrequest);

	parsesuccess = zplay_request.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		errorcode = E_BAD_REQUEST;
		goto exit;
	}

	errorcode = TranferToCommonRequest(zplay_request, comrequest);
	if (errorcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "TranferToCommonRequest failed!");
		goto exit;
	}

	flow_bid.set_req(comrequest);

	if (comrequest.device.location == 0 || comrequest.device.location > 1156999999 || comrequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			comrequest.id.c_str(), comrequest.device.ip.c_str(), comrequest.device.location);
		errorcode = E_REQUEST_DEVICE_IP;
	}
	else
	{
		ferrcode = bid_filter_response(ctx, index, comrequest, 
			callback_process_crequest, NULL, callback_check_price, filter_bid_ext, 
			&comresponse, &adxinfo);
		str_ferrcode = bid_detail_record(600, 10000, ferrcode);
		cflog(g_logid_local, LOGDEBUG, (char*)(string(comrequest.id + ": " + str_ferrcode).c_str()));
		errorcode = ferrcode.errcode;
		flow_bid.set_bid_flow(ferrcode);
		g_ser_log.send_bid_log(index, comrequest, comresponse, errorcode);
	}

	if (errorcode == E_REDIS_CONNECT_INVALID)
	{
		syslog(LOG_INFO, "redis connect invalid");
		kill(getpid(), SIGTERM);
	}

exit:
	if (errorcode == E_SUCCESS)
	{
		uint32_t outputlength = 0;

		TranferToZplayResponse(comrequest, comresponse, &adxinfo, zplay_response);

		outputlength = zplay_response.ByteSize();
		outputdata = (char *)calloc(1, sizeof(char) * (outputlength + 1));

		if ((outputdata != NULL) && (outputlength > 0))
		{
			zplay_response.SerializeToArray(outputdata, outputlength);
			senddata = "Content-Length: " + intToString(outputlength) + "\r\n\r\n" + string(outputdata, outputlength);
			writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
			writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", comresponse.bidid.c_str(), 
				comresponse.seatbid[0].bid[0].adid.c_str(), comrequest.device.deviceid.c_str(), 
				comrequest.device.deviceidtype, comresponse.seatbid[0].bid[0].price);
			numcount[index]++;

			if (numcount[index] % 1000 == 0)
			{
				string str_response;
				google::protobuf::TextFormat::PrintToString(zplay_response, &str_response);
				cflog(g_logid_local, LOGDEBUG, "response=%s", str_response.c_str());
				numcount[index] = 0;
			}

			zplay_response.clear_seatbid();
		}
	}
	else
	{
		senddata = "Status: 204 OK\r\n\r\n";
	}

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, comrequest.id.c_str(), lasttime, errorcode, str_ferrcode.c_str());

	if (recvdata->data != NULL)
	{
		free(recvdata->data);
		recvdata->data = NULL;
	}

	if (recvdata != NULL)
	{
		free(recvdata);
		recvdata = NULL;
	}

	if (outputdata != NULL)
	{
		free(outputdata);
		outputdata = NULL;
	}

	flow_bid.set_err(errorcode);
	return errorcode;
}
