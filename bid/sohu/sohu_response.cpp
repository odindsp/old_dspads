#include <string>
#include <time.h>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <google/protobuf/text_format.h>
#include "../../common/json_util.h"
#include "../../common/bid_filter_dump.h"
#include "sohuRTB.pb.h"
#include "sohu_response.h"

extern map<int64, vector<uint32_t> > sohu_app_cat_table;
extern map<string, vector<uint32_t> > adv_cat_table_adx2com;
extern uint64_t g_logid, g_logid_local, g_logid_response;
extern uint64_t geodb;
extern int *numcount;

extern map<string, vector<pair<string, COM_NATIVE_REQUEST_OBJECT> > > sohu_template_table;
string g_size[] = { "480x152", "640x203", "1080x342", "480x75", "640x100", "1280x200", "120x24" };
set<string> setsize(g_size, g_size + 7);


static bool callback_process_crequest(INOUT COM_REQUEST &crequest, IN const ADXTEMPLATE &adxtemplate)
{
	double ratio = adxtemplate.ratio;

	if ((adxtemplate.adx == ADX_UNKNOWN) || ((ratio > -0.000001) && (ratio < 0.000001)))
	{
		return false;
	}

	for (int i = 0; i < crequest.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[i];
		cimp.bidfloor /= ratio;
	}

	return true;
}

static bool filter_group_ext(IN const COM_REQUEST &crequest, IN const GROUPINFO_EXT &ext)
{
	if (ext.advid != "")
		return true;

	return false;
}

static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &exts)
{
	bool filter = false;
	json_t *root = NULL, *label = NULL;

	if (crequest.imp.size() == 0)
		goto exit;

	if (exts.id == "")
	{
		goto exit;
	}

	json_parse_document(&root, exts.ext.c_str());
	if (root == NULL)
	{
		goto exit;
	}

	if ((label = json_find_first_label(root, "adxtype")) != NULL && label->child->type == JSON_NUMBER)
	{
		int adxtype = atoi(label->child->text);
		if (adxtype != crequest.imp[0].ext.advid)
			goto exit;
	}
	else
		goto exit;

	if (crequest.imp[0].ext.acceptadtype.size() == 0)
	{
		filter = true;
		goto exit;
	}
	if (crequest.imp[0].ext.acceptadtype.size() == 1 && crequest.imp[0].ext.acceptadtype.count(0))
	{
		goto exit;
	}
	if ((label = json_find_first_label(root, "adtype")) != NULL)
	{
		json_t *temp = label->child->child;

		set<uint32_t> pxenecat;

		while (temp != NULL)
		{
			int adtype = atoi(temp->text);

			//if (crequest.imp[0].ext.acceptadtype.count(adtype))
			//{
			//  filter = true;
			//  break;
			//}

			pxenecat.insert(adtype);
			temp = temp->next;
		}

		filter = find_set_cat(g_logid_local, crequest.imp[0].ext.acceptadtype, pxenecat);
	}

exit:
	if (root != NULL)
		json_free_value(&root);
	return filter;
}

static bool callback_check_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if (price >= crequest.imp[0].bidfloor)
		return true;
	else
		return false;
}

void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int64, vector<uint32_t> > &table = *((map<int64, vector<uint32_t> > *)data);

	uint32_t com_cat;
	int64 adx_key;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x
	sscanf(key_start.c_str(), "%lld", &adx_key);

	vector<uint32_t> &s_val = table[adx_key];

	s_val.push_back(com_cat);
}

void callback_insert_pair_string_hex(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

static int TranferToCommonRequest(IN sohuadx::Request &sohu_request, OUT COM_REQUEST &crequest, OUT sohuadx::Response &sohu_response)
{
	int errorcode = E_SUCCESS;

	if (sohu_request.version())
	{
		sohu_response.set_version(sohu_request.version());
	}
	else
	{
		errorcode = E_BAD_REQUEST;
		goto exit;
	}

	if (sohu_request.bidid() != "")
	{
		sohu_response.set_bidid(sohu_request.bidid());
		crequest.id = sohu_request.bidid();
	}
	else
	{
		errorcode = E_REQUEST_NO_BID;
		goto exit;
	}

	if (sohu_request.istest())
	{
		writeLog(g_logid_local, LOGINFO, "%s,This taffic is for testing!", crequest.id.c_str());
	}

	if (sohu_request.impression_size() > 0)
	{
		for (int i = 0; i < sohu_request.impression_size(); i++)
		{
			COM_IMPRESSIONOBJECT imp;

			init_com_impression_object(&imp);

			const sohuadx::Request_Impression &sohu_imp = sohu_request.impression(i);

			imp.bidfloor = sohu_imp.bidfloor() / 100; //转化为元/CPM
			imp.bidfloorcur = "CNY";

			char tempid[1024] = "";
			sprintf(tempid, "%d", sohu_imp.idx());
			imp.id = tempid;

			if (sohu_imp.has_screenlocation())
			{
				if (sohu_imp.screenlocation() == sohuadx::Request_Impression::FIRSTVIEW)
					imp.adpos = ADPOS_FIRST;
				else if (sohu_imp.screenlocation() == sohuadx::Request_Impression::OTHERVIEW)
					imp.adpos = ADPOS_OTHER;
			}

			if (sohu_imp.has_banner())
			{
				int w = sohu_imp.banner().width();
				int h = sohu_imp.banner().height();
				writeLog(g_logid_local, LOGINFO, "%s,banner w=%d,h=%d", crequest.id.c_str(), w, h);
				char bannersize[1024];
				sprintf(bannersize, "%dx%d", w, h);
				string template_ = sohu_imp.banner().template_();
				writeLog(g_logid_local, LOGINFO, "%s,template_=%s", crequest.id.c_str(), template_.c_str());
				if (template_ == "")
				{
					imp.type = IMP_BANNER;
					imp.banner.w = w;
					imp.banner.h = h;
				}
				else if (template_ == "picturetxt" && setsize.count(bannersize))  // only a picture
				{
					imp.type = IMP_BANNER;
					imp.banner.w = w;
					imp.banner.h = h;
					writeLog(g_logid_local, LOGINFO, "%s,find bannersize:%s", crequest.id.c_str(), bannersize);
				}
				else if (sohu_template_table.count(template_))
				{
					imp.type = IMP_NATIVE;
					init_com_native_request_object(imp.native);
					vector<pair<string, COM_NATIVE_REQUEST_OBJECT> > &tmp = sohu_template_table[template_];
					for (int k = 0; k < tmp.size(); ++k)
					{
						if (tmp[k].first == bannersize)
						{
							imp.native = tmp[k].second;
							break;
						}
					}
				}
				else
				{
					errorcode = E_REQUEST_BANNER;
					writeLog(g_logid_local, LOGERROR, "%s,not valid banner type", crequest.id.c_str());
				}
			}
			else if (sohu_imp.has_video())
			{
				imp.type = IMP_VIDEO;
				//imp.video.mimes.insert(sohu_request.impression(i).video().mimes());
				if (sohu_imp.pid() == "90001")
					imp.video.display = DISPLAY_FRONT_PASTER;
				else if (sohu_imp.pid() == "90003")
					imp.video.display = DISPLAY_MIDDLE_PASTER;
				else if (sohu_imp.pid() == "90004")
					imp.video.display = DISPLAY_BACK_PASTER;
				else
				{
					errorcode = E_REQUEST_VIDEO;
					writeLog(g_logid_local, LOGERROR, "%s,not vaild video type", crequest.id.c_str());
				}
				imp.video.maxduration = sohu_imp.video().durationlimit();

				imp.video.minduration = 0;

				imp.video.w = sohu_imp.video().width();
				imp.video.h = sohu_imp.video().height();

				imp.video.mimes.insert(VIDEOFORMAT_MP4);
				writeLog(g_logid_local, LOGINFO, "%s,video w=%d,h=%d", crequest.id.c_str(), imp.video.w, imp.video.h);
				/* pageurl category title external */
			}
			else
			{
				errorcode = E_REQUEST_IMP_TYPE;
				writeLog(g_logid_local, LOGERROR, "%s,not video and banner", crequest.id.c_str());
			}

			for (int j = 0; j < sohu_imp.acceptadvertisingtype_size(); j++)
			{
				string adtype = sohu_imp.acceptadvertisingtype(j);
				/*if (adtype == "101000")
				  imp.ext.acceptadtype.insert(ACCEPT_ADTYPE_BRAND);
				  else if (adtype == "102101")
				  imp.ext.acceptadtype.insert(ACCEPT_ADTYPE_EFFECT_ELECTTRADE);
				  else if (adtype == "102102")
				  imp.ext.acceptadtype.insert(ACCEPT_ADTYPE_EFFECT_GAME);
				  else if (adtype == "102100")
				  imp.ext.acceptadtype.insert(ACCEPT_ADTYPE_EFFECT_OTHER);*/

				// 转换成支持的广告行业类型
				writeLog(g_logid_local, LOGINFO, "%s,adtype=%s", crequest.id.c_str(), adtype.c_str());
				vector<uint32_t> &v_cat = adv_cat_table_adx2com[adtype];
				for (int k = 0; k < v_cat.size(); ++k)
				{
					imp.ext.acceptadtype.insert(v_cat[k]);
				}
			}
			if (sohu_imp.acceptadvertisingtype_size() > 0 && imp.ext.acceptadtype.size() == 0)
			{
				imp.ext.acceptadtype.insert(0);
				writeLog(g_logid_local, LOGINFO, "%s,adtype not find our need", crequest.id.c_str());
			}

			imp.requestidlife = 3600;
			crequest.imp.push_back(imp);
			break;
		}
	}
	else
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "%s,not has imp", crequest.id.c_str());
	}

	/* User section now don't use */

	if (sohu_request.has_device())
	{
		if (!strcasecmp(sohu_request.device().type().c_str(), "mobile")) //wap was in wireless Device
		{

			crequest.device.ip = sohu_request.device().ip();
			if (crequest.device.ip != "")
			{
				crequest.device.location = getRegionCode(geodb, crequest.device.ip);
			}

			string ua = sohu_request.device().ua().c_str();
			cout << "ua:" << ua << endl;

			size_t found = ua.rfind("|");
			if (found != string::npos)
			{
				crequest.device.osv = ua.substr(found + 1);
			}

			//sscanf(ua, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]", imei, imsi, mac, idfa, osv);

			string devid = sohu_request.device().imei();
			if (devid != "")
			{
				transform(devid.begin(), devid.end(), devid.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_IMEI_MD5, devid));
			}

			devid = sohu_request.device().mac();
			if (devid != "")
			{
				transform(devid.begin(), devid.end(), devid.begin(), ::tolower);
				crequest.device.dids.insert(make_pair(DID_MAC_MD5, devid));
			}

			devid = sohu_request.device().idfa();
			if (devid != "")
			{
				crequest.device.dpids.insert(make_pair(DPID_IDFA, devid));
			}

			devid = sohu_request.device().androidid();
			if (devid != "")
			{
				transform(devid.begin(), devid.end(), devid.begin(), ::tolower);
				crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, devid));
			}

			/*carrier infomation don't comfirm*/

			if (!strcasecmp(sohu_request.device().nettype().c_str(), "2G"))
				crequest.device.connectiontype = CONNECTION_CELLULAR_2G;
			else if (!strcasecmp(sohu_request.device().nettype().c_str(), "3G"))
				crequest.device.connectiontype = CONNECTION_CELLULAR_3G;
			else if (!strcasecmp(sohu_request.device().nettype().c_str(), "4G"))
				crequest.device.connectiontype = CONNECTION_CELLULAR_4G;
			else if (!strcasecmp(sohu_request.device().nettype().c_str(), "WIFI"))
				crequest.device.connectiontype = CONNECTION_WIFI;

			string device_os = sohu_request.device().mobiletype();
			if (device_os != "")
			{
				transform(device_os.begin(), device_os.end(), device_os.begin(), ::tolower);
				if (device_os.find("ipad") != string::npos)
				{
					crequest.device.os = OS_IOS;
					crequest.device.devicetype = DEVICE_TABLET;
					crequest.device.make = APPLE_MAKE;
					crequest.device.makestr = "apple";
					//crequest.is_secure = 1;
				}
				else if (device_os.find("iphone") != string::npos)
				{
					crequest.device.os = OS_IOS;
					crequest.device.devicetype = DEVICE_PHONE;
					crequest.device.make = APPLE_MAKE;
					crequest.device.makestr = "apple";
					//crequest.is_secure = 1;
				}
				else if (device_os.find("androidphone") != string::npos)
				{
					crequest.device.os = OS_ANDROID;
					crequest.device.devicetype = DEVICE_PHONE;
					crequest.device.make = DEFAULT_MAKE;
					crequest.device.makestr = "other";
					crequest.is_secure = 0;
				}
				else if (device_os.find("androidpad") != string::npos)
				{
					crequest.device.os = OS_ANDROID;
					crequest.device.devicetype = DEVICE_TABLET;
					crequest.device.make = DEFAULT_MAKE;
					crequest.device.makestr = "other";
					crequest.is_secure = 0;
				}
			}
		}
		else
		{
			errorcode = E_REQUEST_DEVICE_TYPE;
			writeLog(g_logid_local, LOGERROR, "%s,not mobile", crequest.id.c_str());
		}
	}
	else
	{
		errorcode = E_REQUEST_DEVICE_TYPE;
		writeLog(g_logid_local, LOGERROR, "%s,not has device", crequest.id.c_str());
	}

	if (sohu_request.has_site())
	{
		int64 appcat = sohu_request.site().category();

		crequest.app.name = sohu_request.site().name();
		crequest.app.id = crequest.app.name;
		if (crequest.imp.size() > 0)
		{
			if (crequest.app.name == "SOHU" || crequest.app.name == "SOHU_NEWS_APP" || crequest.app.name == "SOHU_WAP")
			{
				crequest.imp[0].ext.advid = 1;
			}
			else if (crequest.app.name == "SOHU_TV" || crequest.app.name == "SOHU_TV_APP" || crequest.app.name == "56")
			{
				crequest.imp[0].ext.advid = 2;
			}
		}

		vector<uint32_t> &v_cat = sohu_app_cat_table[appcat];

		for (int j = 0; j < v_cat.size(); j++)
		{
			crequest.app.cat.insert(v_cat[j]);
		}

		/* page category ref 如何使用 */
	}

	if (sohu_request.has_user())
	{
		crequest.user.id = sohu_request.user().suid();
		if (sohu_request.user().searchkeywords_size() > 0)
		{
			crequest.user.searchkey = sohu_request.user().searchkeywords(0);
			for (int j = 1; j < sohu_request.user().searchkeywords_size(); ++j)
			{
				crequest.user.searchkey += ",";
				crequest.user.searchkey += sohu_request.user().searchkeywords(j);
			}
		}
	}
	/* excludeAdCategory需要屏蔽的广告主类型 目前无 */

	if (crequest.device.dids.size() == 0 && crequest.device.dpids.size() == 0)
	{
		errorcode = E_REQUEST_DEVICE_ID;
		writeLog(g_logid_local, LOGERROR, "%s,dids and dpids size is zero!", crequest.id.c_str());
		goto exit;
	}

	if (0 == crequest.imp.size())  // 没有符合的曝光对象
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "%s,imp size is zero!", crequest.id.c_str());
		goto exit;
	}

	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		errorcode = E_REQUEST_DEVICE_IP;
	}

exit:
	crequest.cur.insert(string("CNY"));
	return errorcode;
}

void TranferToSohuResponse(COM_REQUEST comrequest, COM_RESPONSE &comresponse, sohuadx::Response &sohu_response)
{
	string trace_url_par = get_trace_url_par(comrequest, comresponse);
	for (int i = 0; i < comresponse.seatbid.size(); ++i)
	{
		sohuadx::Response_SeatBid *sohu_seat = sohu_response.add_seatbid();
		COM_SEATBIDOBJECT *seatbid = &comresponse.seatbid[i];
		for (int j = 0; j < seatbid->bid.size(); ++j)
		{

			COM_BIDOBJECT *bid = &seatbid->bid[j];
			sohu_seat->set_idx(atoi(bid->impid.c_str()));

			sohuadx::Response_Bid *sohu_bid = sohu_seat->add_bid();

			char ext[MID_LENGTH] = "";

			double price = (double)bid->price * 100;
			sohu_bid->set_price((uint32_t)price);
			sohu_bid->set_adurl(bid->sourceurl);
			sohu_bid->set_ext(trace_url_par.c_str());
		}
	}
}

int sohuResponse(IN uint8_t index, IN uint64_t ctx, IN RECVDATA *recvdata, OUT string &senddata, OUT FLOW_BID_INFO &flow_bid)
{
	int errcode = E_SUCCESS;
	FULL_ERRCODE ferrcode = { 0 };
	char *outputdata = NULL;
	ADXTEMPLATE adxinfo;

	COM_REQUEST comrequest;
	sohuadx::Request sohu_request;

	COM_RESPONSE comresponse;
	sohuadx::Response sohu_response;

	string str_ferrcode = "";

	bool parsesuccess = false;
	struct timespec ts1, ts2;
	long lasttime = 0;
	uint32_t outputlength = 0;
	getTime(&ts1);

	if ((recvdata == NULL) || (ctx == 0))
	{
		errcode = E_BAD_REQUEST;
		writeLog(g_logid_local, LOGERROR, string("recvdaa or ctx is NULL"));
		goto exit;
	}

	if ((recvdata->data == NULL) || (recvdata->length == 0))
	{
		errcode = E_BAD_REQUEST;
		writeLog(g_logid_local, LOGERROR, string("recvdata->data is null or recvdata->length is 0"));
		goto exit;
	}

	parsesuccess = sohu_request.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		errcode = E_BAD_REQUEST;
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		goto exit;
	}

	init_com_template_object(&adxinfo);
	init_com_message_request(&comrequest);

	errcode = TranferToCommonRequest(sohu_request, comrequest, sohu_response);

	if (errcode == E_BAD_REQUEST)
	{
		writeLog(g_logid_local, LOGERROR, "request is bad!");
		goto exit;
	}
	else if (errcode != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "bid:%s,request is valid for bidding!", sohu_request.bidid().c_str());
		goto returnresponse;
	}

	flow_bid.set_req(comrequest);
	ferrcode = bid_filter_response(ctx, index, comrequest, 
		callback_process_crequest, filter_group_ext, callback_check_price, filter_bid_ext, 
		&comresponse, &adxinfo);

	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	errcode = ferrcode.errcode;
	flow_bid.set_bid_flow(ferrcode);
	g_ser_log.send_bid_log(index, comrequest, comresponse, errcode);

	if (errcode == E_SUCCESS)
	{
		TranferToSohuResponse(comrequest, comresponse, sohu_response);
		writeLog(g_logid, LOGDEBUG, string(recvdata->data, recvdata->length));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", comrequest.id.c_str(),
			comresponse.seatbid[0].bid[0].mapid.c_str(), comrequest.device.deviceid.c_str(),
			comrequest.device.deviceidtype, comresponse.seatbid[0].bid[0].price);
		numcount[index]++;
		if (numcount[index] % 1000 == 0)
		{
			string str_response;
			google::protobuf::TextFormat::PrintToString(sohu_response, &str_response);
			writeLog(g_logid_local, LOGDEBUG, "response=%s", str_response.c_str());
			numcount[index] = 0;
		}
	}

returnresponse:
	outputlength = sohu_response.ByteSize();
	outputdata = (char *)calloc(1, sizeof(char) * (outputlength + 1));
	if ((outputdata != NULL))
	{
		sohu_response.SerializeToArray(outputdata, outputlength);
		sohu_response.clear_seatbid();
		senddata = string("Content-Length: ") + intToString(outputlength) + "\r\n\r\n" + string(outputdata, outputlength);
	}

exit:
	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, comrequest.id.c_str(), lasttime, errcode, str_ferrcode.c_str());

	if (outputdata != NULL)
	{
		free(outputdata);
		outputdata = NULL;
	}

	if (recvdata->data != NULL)
		free(recvdata->data);
	if (recvdata != NULL)
		free(recvdata);

	flow_bid.set_err(errcode);
	return errcode;
}
