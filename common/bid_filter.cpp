#include <string>
#include <time.h>
#include <algorithm>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include "type.h"
#include "errorcode.h"
#include "redisoperation.h"
#include "confoperation.h"
#include "selfsemphore.h"
#include "setlog.h"
#include "geohash.h"
#include "filter_by_deviceid_wblist.h"
#include "bid_filter_dump.h"
#include "bid_filter.h"

#define ZERO(x) ((x) > -0.000001 && (x) < 0.000001)
struct _redis_fds
{
	redisContext *redis_wblist_detail;
	redisContext *redis_id;
	redisContext *redis_gr;//group frequency port
	redisContext *redis_ur;//user frequency port
	redisContext *redis_user_advcat;//intelligent recommend relation data;
};
typedef struct _redis_fds REDIS_FDS;

typedef struct _BID_INFO
{
	int finish_time;
	double curr_ratio;
}BID_INFO;

struct filter_context
{
	uint8_t adx;
	bool is_print_time;
	uint64_t logid;

	string settle_server;
	string click_server;
	string impression_server;
	string active_server;

	string settle_https_server;
	string click_https_server;
	string impression_https_server;
	string active_https_server;

	uint64_t cache;

	redisContext *redis_smart;
	vector<REDIS_FDS> redis_fds;
	redisContext *redis_relation_advcat;//intelligent recommend relation data;
	map<string, double> advcat;
	pthread_mutex_t advcat_mutex;
	int timer_recommend;

	vector<string> v_cmd_smart;
	pthread_mutex_t v_cmd_smart_mutex;
	pthread_t t_pipeline_smart;

	map<string, map<int, BID_INFO> > group_bid_info;   // GROUPID,HOUR
	int zero_flag;
	pthread_mutex_t bid_info_mutex;
	int pipe_err;
	pthread_mutex_t pipe_err_mutex;

	bool *run_flag;
};
typedef struct filter_context FILTERCONTEXT;

int get_cre_by_catadv(IN FILTERCONTEXT *fctx, INOUT COM_REQUEST &crequest, IN redisContext *redis_user_advcat,
	IN const vector<CREATIVEOBJECT> &v_creative, IN const vector<int> &index, OUT map<string, ERRCODE_GROUP> &m_err_group);

void set_last_filter_err(INOUT map<string, ERRCODE_GROUP> &m_err_group, IN const vector<CREATIVEOBJECT> &v_creative,
	IN int sel, IN const vector<int> &index, IN const set<int> &index_advcat);

static int check_did_validity(IN const uint8_t &didtype, IN const string &did)
{
	int errcode = E_SUCCESS;

	switch (didtype)
	{
	case DID_IMEI:
		if ((!check_string_type(did, LEN_IMEI_SHORT, false, false, INT_RADIX_16)) && 
			(!check_string_type(did, LEN_IMEI_LONG, false, false, INT_RADIX_16)))//16进制数字
		{
			errcode = E_REQUEST_DEVICE_ID_DID_IMEI;
			goto exit;
		}
		break;

	case DID_MAC://mac具体形式未知
		if ((!check_string_type(did, LEN_MAC_SHORT, false, false, INT_RADIX_16)) && 
			(!check_string_type(did, LEN_MAC_LONG, true, true, INT_RADIX_16)))
		{
			errcode = E_REQUEST_DEVICE_ID_DID_MAC;
			goto exit;
		}
		break;

	case DID_IMEI_SHA1:
	case DID_MAC_SHA1:
		if (!check_string_type(did, LEN_SHA1, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_REQUEST_DEVICE_ID_DID_SHA1;
			goto exit;
		}
		break;

	case DID_IMEI_MD5:
	case DID_MAC_MD5:
		if (!check_string_type(did, LEN_MD5, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_REQUEST_DEVICE_ID_DID_MD5;
			goto exit;
		}
		break;

	case DID_OTHER:
		if (did.length() == 0)
		{
			errcode = E_REQUEST_DEVICE_ID_DID_OTHER;
			goto exit;
		}
		break;

	default:
		{
			errcode = E_REQUEST_DEVICE_ID_DID_UNKNOWN;
			goto exit;
		}
		break;
	}

exit:

	return errcode;
}

static int check_dpid_validity(IN const uint8_t &dpidtype, IN const string &dpid)
{
	int errcode = E_SUCCESS;

	switch (dpidtype)
	{
	case DPID_ANDROIDID:
		if (!check_string_type(dpid, LEN_ANDROIDID, false, false, INT_RADIX_16))//16位且都是数字
		{
			errcode = E_REQUEST_DEVICE_ID_DPID_ANDROIDID;
			goto exit;
		}
		break;

	case DPID_IDFA:
		if (!check_string_type(dpid, LEN_IDFA, true, false, INT_RADIX_16))//必须是字母数字
		{
			errcode = E_REQUEST_DEVICE_ID_DPID_IDFA;
			goto exit;
		}
		break;

	case DPID_ANDROIDID_SHA1:
	case DPID_IDFA_SHA1:
		if (!check_string_type(dpid, LEN_SHA1, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_REQUEST_DEVICE_ID_DPID_SHA1;
			goto exit;
		}
		break;

	case DPID_ANDROIDID_MD5:
	case DPID_IDFA_MD5:
		if (!check_string_type(dpid, LEN_MD5, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_REQUEST_DEVICE_ID_DPID_MD5;
			goto exit;
		}
		break;

	case DPID_OTHER:
		if (dpid.length() == 0)
		{
			errcode = E_REQUEST_DEVICE_ID_DPID_OTHER;
			goto exit;
		}
		break;

	default:
		{
			errcode = E_REQUEST_DEVICE_ID_DPID_UNKNOWN;
			goto exit;
		}
		break;
	}

exit:

	return errcode;
}

int check_native_request(COM_NATIVE_REQUEST_OBJECT &native)
{
	int errcode = E_SUCCESS;

	if (native.layout != NATIVE_LAYOUTTYPE_NEWSFEED && 
		native.layout != NATIVE_LAYOUTTYPE_CONTENTSTREAM && 
		native.layout != NATIVE_LAYOUTTYPE_OPENSCREEN)
	{
		errcode = E_REQUEST_IMP_NATIVE_LAYOUT;
		goto exit;
	}

	if (native.plcmtcnt < 1)
	{
		errcode = E_REQUEST_IMP_NATIVE_PLCMTCNT;
		goto exit;
	}

	for (int i = 0; i < native.assets.size();)
	{
		int errcode_tmp = E_SUCCESS;

		const COM_ASSET_REQUEST_OBJECT &asset = native.assets[i];

		const uint8_t &required = asset.required;

		const uint8_t &type = asset.type;

		switch (type)
		{
		case NATIVE_ASSETTYPE_TITLE:
			{
				const COM_TITLE_REQUEST_OBJECT &title = asset.title;

				if (title.len == 0)
				{
					errcode_tmp = E_REQUEST_IMP_NATIVE_ASSET_TITLE_LENGTH;
				}
			}
			break;

		case NATIVE_ASSETTYPE_IMAGE:
			{
				const COM_IMAGE_REQUEST_OBJECT &img = asset.img;

				if ((img.type == ASSET_IMAGETYPE_ICON) || (img.type == ASSET_IMAGETYPE_MAIN))
				{
					if(img.type == ASSET_IMAGETYPE_MAIN)
					{
						native.img_num++;
					}
					if ((img.w && img.h) == 0)
					{
						errcode_tmp = E_REQUEST_NATIVE_ASSET_IMAGE_SIZE;
					}
				}
				else
				{	
					errcode_tmp = E_REQUEST_IMP_NATIVE_ASSET_IMAGE_TYPE;
				}
			}
			break;

		case NATIVE_ASSETTYPE_DATA:
			{
				const COM_DATA_REQUEST_OBJECT &data = asset.data;

				if ((data.type != ASSET_DATATYPE_DESC) && (data.type != ASSET_DATATYPE_RATING) && (data.type != ASSET_DATATYPE_CTATEXT))
				{
					errcode_tmp = E_REQUEST_IMP_NATIVE_ASSET_DATA_TYPE;
				}
				else if (data.type == ASSET_DATATYPE_DESC && data.len == 0)
				{
					errcode_tmp = E_REQUEST_IMP_NATIVE_ASSET_DATA_LENGTH;
				}
			}
			break;

		default:
			{
				errcode_tmp = E_REQUEST_IMP_NATIVE_ASSET_TYPE;
			}
			break;
		}

		if (errcode_tmp == E_SUCCESS)//成功，则检查下一个
		{
			i++;
		}
		else//发现错误的asset
		{
			if (required == 1)//必选，则中断竞价
			{
				errcode = errcode_tmp;
				goto exit;
			}
			else//可选，则去掉此asset
			{
				//pop
				native.assets.erase(native.assets.begin() + i);
			}
		}
	}

	if (native.assets.size() < 2)
	{
		errcode = E_REQUEST_IMP_NATIVE_ASSETCNT;//at least: title and image
		goto exit;
	}

exit:

	return errcode;
}

static int check_crequest_validity(IN COM_REQUEST &crequest)
{
	int errcode = E_SUCCESS;

	map<uint8_t, string>::const_iterator it;
	int location = 0;

	// 5. 长度定义需要商量 11-11
	if (crequest.id.length() > MAX_LEN_REQUESTID)//tanx:32 amax:33 inmobi(uuid):36
	{
		errcode = E_REQUEST_ID;
		goto exit;
	}

	if (crequest.imp.size() == 0)
	{
		errcode = E_REQUEST_IMP_SIZE_ZERO;
		goto exit;
	}

	for (size_t i = 0; i < crequest.imp.size(); i++)
	{
		string &impid = crequest.imp[i].id;
		if (impid.empty()){
			impid = string("recvimp") + uintToString(i);
		}
	}

	location = crequest.device.location;
	if (location > 1156999999 || location < CHINAREGIONCODE)
	{
		//cflog(logid, LOGDEBUG, "%s, getRegionCode ip:%s location:%d failed!", crequest.id.c_str(), crequest.device.ip.c_str(), location);
		errcode = E_REQUEST_DEVICE_IP;
		goto exit;
	}

	if ((crequest.device.os != OS_IOS) && (crequest.device.os != OS_ANDROID))
	{
		errcode = E_REQUEST_DEVICE_OS;
		goto exit;
	}

	if(crequest.device.devicetype != DEVICE_PHONE && crequest.device.devicetype != DEVICE_TABLET && crequest.device.devicetype != DEVICE_TV)
	{
		errcode = E_REQUEST_DEVICE_TYPE;
		goto exit;
	}

	if ((crequest.device.dpids.size() | crequest.device.dids.size()) == 0)
	{
		errcode = E_REQUEST_DEVICE_ID;
		goto exit;
	}

	//支持多个dpid或多个did
	for (it = crequest.device.dpids.begin(); it != crequest.device.dpids.end(); it++)
	{
		errcode = check_dpid_validity(it->first, it->second);
		if (errcode != E_SUCCESS)
		{
			goto exit;
		}	
	}

	for (it = crequest.device.dids.begin(); it != crequest.device.dids.end(); it++)
	{
		errcode = check_did_validity(it->first, it->second);
		if (errcode != E_SUCCESS)
		{
			goto exit;
		}	
	}
	//支持多个dpid或多个did

	if (crequest.imp[0].type == IMP_NATIVE)
	{
		errcode = check_native_request(crequest.imp[0].native);
		if (errcode != E_SUCCESS)
		{
			goto exit;
		}	
	}

	if(crequest.at != BID_RTB && crequest.at != BID_PMP)
	{
		errcode = E_REQUEST_INVALID_BID_TYPE;
		goto exit;
	}

	if(crequest.device.os == OS_IOS)
	{
		crequest.device.make = APPLE_MAKE;
	}
	else if(crequest.device.makestr == "")
	{
		errcode = E_REQUEST_DEVICE_MAKE_NIL;
		goto exit;
	}

	if(crequest.device.os != OS_IOS && crequest.device.make == APPLE_MAKE)
	{
		errcode = E_REQUEST_DEVICE_MAKE_OS;
		goto exit;
	}
exit:

	return errcode;
}

static int check_adxtemplate_validity(IN const ADXTEMPLATE &adxtemp, IN const COM_REQUEST &crequest, IN const COM_BIDOBJECT &cbid)
{
	int errcode = E_ADXTEMPLATE_NOT_FIND_ADM;

	if (adxtemp.adx == ADX_UNKNOWN)
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_ADX;
		goto exit;
	}

	if ((adxtemp.ratio > -0.000001) && (adxtemp.ratio < 0.000001))
	{
		errcode = E_ADXTEMPLATE_RATIO_ZERO;    // 修改错误码 11-13
		goto exit;
	}

	if (adxtemp.iurl.size() == 0)
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_IURL;
		goto exit;
	}

	if (adxtemp.cturl.size() == 0)
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_CTURL;
		goto exit;
	}

	if (adxtemp.aurl == "")
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_AURL;
		goto exit;
	}

	if (adxtemp.nurl == "")
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_NURL;
		goto exit;
	}

	if (crequest.imp[0].type == IMP_BANNER)
	{
		for (int i = 0; i < adxtemp.adms.size(); i++)
		{
			if ((adxtemp.adms[i].os == crequest.device.os) && (adxtemp.adms[i].type == cbid.type)/* && (adxtemp.adms[i].adm != "")*/)
			{
				errcode = E_SUCCESS;
				break;
			}
		}
	}
	else
		errcode = E_SUCCESS;

exit:

	return errcode;
}

static void clear_ur(INOUT USER_FREQUENCY_COUNT &ur)
{
	ur.userid = "";
	ur.user_group_frequency_count.clear();
}

static void* thread_redis_pipeline_smart(void *arg)
{
	FILTERCONTEXT *fctx = (FILTERCONTEXT *)arg;

	do
	{
		usleep(30000);

		struct timespec ts1;

		getTime(&ts1);
		pthread_mutex_lock(&fctx->v_cmd_smart_mutex);
		long cmds_size = fctx->v_cmd_smart.size();

		if (cmds_size > 0)
		{
			vector<string> cmds_temp;
			cmds_temp.swap(fctx->v_cmd_smart);

			pthread_mutex_unlock(&fctx->v_cmd_smart_mutex);

			for(unsigned long i = 0; i < cmds_size; ++i)
				redisAppendCommand(fctx->redis_smart, cmds_temp[i].c_str());

			for(unsigned long i = 0; i < cmds_size; ++i)
			{
				redisReply *reply = NULL;

				redisGetReply(fctx->redis_smart, (void **)&reply);

				if (reply != NULL)
				{
					if (reply->type == REDIS_REPLY_ERROR)
					{
						cflog(fctx->logid, LOGDEBUG, "redisGetReply failed! The commands is %s", cmds_temp[i].c_str());
					}
				}
				else
				{
					pthread_mutex_lock(&fctx->pipe_err_mutex);
					fctx->pipe_err = E_REDIS_CONNECT_INVALID;
					pthread_mutex_unlock(&fctx->pipe_err_mutex);
					cflog(fctx->logid, LOGDEBUG, "write smart bid redis context invalid!");
				}

				if(reply != NULL)
					freeReplyObject(reply);
			}

			record_cost(fctx->logid, fctx->is_print_time, " ", string("pipeline smart size:"+intToString(cmds_size)).c_str(), ts1);
		}
		else
			pthread_mutex_unlock(&fctx->v_cmd_smart_mutex);

	}while(*fctx->run_flag);

	cout<<"thread_redis_pipeline_smart exit"<<endl;
	return NULL;
}

static int get_random_index(int module)
{
	srand((unsigned)time(NULL));

	return rand() % module;
}

static void ReplaceMonitor(MD5_CTX &hash_ctx, IN COM_REQUEST &request, INOUT string &url)
{
	if (url.find("miaozhen.com") != string::npos)
	{
		replaceMiaozhenMonitor(hash_ctx, request.device, url);
	}
	else if (url.find("admaster") != string::npos)
	{
		replaceAdmasterMonitor(hash_ctx, request.device, url, request.id);
	}
	else if (url.find("nielsen") != string::npos || url.find("imrworldwide") != string::npos)
	{
		replaceNielsenMonitor(hash_ctx, request.device, url);
	}
	else if (url.find("gtrace.gimc") != string::npos)
	{
		replaceGimcMonitor(hash_ctx, request.device, url);
	}
}

static void ReplaceThirdMonitor(IN COM_REQUEST &req, INOUT COM_BIDOBJECT &com_bidobject)
{
	MD5_CTX hash_ctx;
	MD5_Init(&hash_ctx);

	ReplaceMonitor(hash_ctx, req, com_bidobject.curl);
	replace_url(req.id, com_bidobject.mapid, req.device.deviceid, req.device.deviceidtype, com_bidobject.curl);
	if (com_bidobject.effectmonitor == 1){
		replaceCurlParam(req.id, req.device, com_bidobject.mapid, com_bidobject.curl);
	}

	ReplaceMonitor(hash_ctx, req, com_bidobject.monitorcode);

	for (size_t i = 0; i < com_bidobject.imonitorurl.size(); i++)
	{
		ReplaceMonitor(hash_ctx, req, com_bidobject.imonitorurl[i]);
	}

	for (size_t i = 0; i < com_bidobject.cmonitorurl.size(); i++)
	{
		ReplaceMonitor(hash_ctx, req, com_bidobject.cmonitorurl[i]);
	}
}

static void set_bid_object(IN const COM_IMPRESSIONOBJECT &imp, IN const CREATIVEOBJECT &creative, IN const GROUPINFO &groupinfo, IN int flag, 
						   OUT COM_BIDOBJECT &com_bidobject)
{
	int k = 0;
	int i;
	string str1 = "";
	string str2 = "";
	int is_secure = flag;

	com_bidobject.cat = groupinfo.cat;//copy from groupinfo
	com_bidobject.adomain = groupinfo.adomain;//copy from groupinfo

	com_bidobject.impid = imp.id;
	com_bidobject.price = creative.price;
	com_bidobject.mapid = creative.mapid;
	com_bidobject.adid = creative.mapid;
	com_bidobject.groupid = creative.groupid;
	com_bidobject.type = creative.type;
	com_bidobject.ctype = creative.ctype;
	com_bidobject.ftype = creative.ftype;

	com_bidobject.bundle = creative.bundle;
	com_bidobject.apkname = creative.apkname;
	com_bidobject.w = creative.w;
	com_bidobject.h = creative.h;

	if(is_secure == 1)
	{
		str1 = "http:";
		str2 = "https:";
		com_bidobject.curl = creative.securecurl;
	}
	else
	{
		com_bidobject.curl = creative.curl;
	}

	com_bidobject.landingurl = creative.landingurl;
	com_bidobject.redirect = groupinfo.redirect;//copy from groupinfo
	com_bidobject.effectmonitor = groupinfo.effectmonitor;
	com_bidobject.dealid = creative.dealid;

	com_bidobject.cmonitorurl = creative.cmonitorurl;
	com_bidobject.imonitorurl = creative.imonitorurl;
	com_bidobject.monitorcode = creative.monitorcode;
	com_bidobject.sourceurl = creative.sourceurl;

	com_bidobject.cid = creative.cid;
	com_bidobject.crid = creative.crid;

	com_bidobject.attr = creative.attr;
	com_bidobject.groupinfo_ext = groupinfo.ext;
	com_bidobject.creative_ext = creative.ext;

	if(is_secure == 1)
	{
		replaceMacro(com_bidobject.sourceurl,str1.c_str(),str2.c_str());
		replaceMacro(com_bidobject.landingurl,str1.c_str(),str2.c_str());
		replaceMacro(com_bidobject.monitorcode,str1.c_str(),str2.c_str());

		for(i = 0; i < com_bidobject.imonitorurl.size(); ++i)
		{
			replaceMacro(com_bidobject.imonitorurl[i],str1.c_str(),str2.c_str());
		}
		for(i = 0; i < com_bidobject.cmonitorurl.size(); ++i)
		{
			replaceMacro(com_bidobject.cmonitorurl[i],str1.c_str(),str2.c_str());
		}
	}

	if (imp.type == IMP_NATIVE)
	{
		vector<COM_ASSET_RESPONSE_OBJECT> &assets_response = com_bidobject.native.assets;

		const vector<COM_ASSET_REQUEST_OBJECT> &assets = imp.native.assets;

		for (int i = 0; i < assets.size(); i++)
		{
			COM_ASSET_RESPONSE_OBJECT asset_out;
			init_com_asset_response_object(asset_out);

			const COM_ASSET_REQUEST_OBJECT &asset = assets[i];

			asset_out.id = asset.id;

			asset_out.required = asset.required;

			asset_out.type = asset.type;

			asset_out.img.type = asset.img.type;

			asset_out.data.type = asset.data.type;

			switch (asset.type)
			{
			case NATIVE_ASSETTYPE_TITLE:
				{
					if (asset.title.len < creative.title.length())//required must be : 0
					{
						assert(asset.required == 0);

						continue;
					}
					else
					{
						asset_out.title.text = creative.title;
					}		
				}
				break;

			case NATIVE_ASSETTYPE_IMAGE:
				{
					if (asset.img.type == ASSET_IMAGETYPE_ICON)
					{
						asset_out.img.w = creative.icon.w;
						asset_out.img.h = creative.icon.h;
						asset_out.img.url = creative.icon.sourceurl;
					}
					else if (asset.img.type == ASSET_IMAGETYPE_MAIN)
					{
						asset_out.img.w = creative.imgs[k].w;
						asset_out.img.h = creative.imgs[k].h;
						asset_out.img.url = creative.imgs[k].sourceurl;
						k++;
					}
					if(is_secure == 1)
					{
						replaceMacro(asset_out.img.url,str1.c_str(),str2.c_str());
					}
				}
				break;

			case NATIVE_ASSETTYPE_DATA:
				{
					if (asset.data.type == ASSET_DATATYPE_DESC)
					{
						if (asset.data.len < creative.description.length())//required must be : 0
						{
							assert(asset.required == 0);

							continue;
						}
						else
						{
							asset_out.data.value = creative.description;
						}		
					}
					else if (asset.data.type == ASSET_DATATYPE_RATING)
					{
						if (creative.rating > 5)
						{
							assert(asset.required == 0);

							continue;
						}
						else
						{
							asset_out.data.label = "Rating";

							asset_out.data.value = intToString(creative.rating);
						}		
					}
					else if (asset.data.type == ASSET_DATATYPE_CTATEXT)
					{
						if (creative.ctatext == "")
						{
							assert(asset.required == 0);

							continue;
						}
						else
						{
							asset_out.data.value = creative.ctatext;
						}		
					}
				}
				break;

			default:
				break;
			}

			assets_response.push_back(asset_out);
		}
	}

	return;
}

static bool adjust_wlist_bid_price(IN REDIS_INFO *redis_cashe, IN COM_REQUEST request, INOUT CREATIVEOBJECT &creative)
{
	double &bidfloor = request.imp[0].bidfloor;

	string &groupid = creative.groupid;

	map<string, WBLIST>::iterator it = redis_cashe->wblist.find(groupid);
	if(it == redis_cashe->wblist.end())
		return false;

	WBLIST &wblist = it->second;
	if (bidfloor > wblist.mprice)
		return false;

	double price = (bidfloor > creative.price ? bidfloor : creative.price);
	price = price * wblist.ratio;//白名单的客户,在底价基础上，无条件乘以一个加价系数
	if (price > wblist.mprice)
		creative.price = wblist.mprice;//加价后不能超过max price
	else if (price > creative.price)
		creative.price = price;

	return true;
}

static void write_smart_ratio(IN FILTERCONTEXT *fctx, IN string &groupid, IN  map<int, BID_INFO> &write_bid_info, IN int day)
{
	char *text = NULL;
	json_t *root = NULL, *label_group = NULL;
	string command;
	root = json_new_object();
	if (root == NULL)
		return;

	label_group = json_new_object();
	char hour[MIN_LENGTH];
	map<int, BID_INFO>::iterator p = write_bid_info.begin();	
	for (p; p != write_bid_info.end(); ++p)
	{
		sprintf(hour, "%02d", p->first);
		if (p->second.curr_ratio > INIT_COMP_RATIO)
			p->second.curr_ratio = 0;
		jsonInsertFloat(label_group, hour, p->second.curr_ratio, 6);
	}

	json_insert_pair_into_object(root, groupid.c_str(), label_group);

	json_tree_to_string(root, &text);
	json_free_value(&root);

	if (text == NULL)
	{
		return;
	}

	char m_day_key[MID_LENGTH] = "";
	sprintf(m_day_key, "SETEX dsp_groupid_smart_%02d_%s_%02d", fctx->adx, groupid.c_str(), day);

	command = string(m_day_key) + " " + intToString(3*DAYSECOND - getCurrentTimeSecond()) + " " + string(text); 

	pthread_mutex_lock(&fctx->v_cmd_smart_mutex);
	fctx->v_cmd_smart.push_back(command);
	pthread_mutex_unlock(&fctx->v_cmd_smart_mutex);

	free(text);

	//	cout<<command<<endl;
	return;
}


static bool adjust_ordinary_bid_price(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN const char *bid, INOUT CREATIVEOBJECT &creative)
{
	timespec ts1;
	getTime(&ts1);
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);
	bool flag = false;
	int last_hour = tm.tm_hour - 1;
	string groupid = creative.groupid;
	map<int, BID_INFO> write_bid_info;

	pthread_mutex_lock(&fctx->bid_info_mutex);
	map<string, map<int, BID_INFO> > &group_bid_info = fctx->group_bid_info;
	//redis_cashe->grsmartbidinfo
	if (group_bid_info.count(groupid))  // 存在该项目
	{
		map<int, BID_INFO> &bid_info = group_bid_info[groupid];
		if (bid_info.count(tm.tm_hour))
		{
			BID_INFO &info = bid_info[tm.tm_hour];
			if (info.curr_ratio < INIT_COMP_RATIO) // 已经设置过price
			{
				va_cout("curr_ratio=%lf,curr_finish_time=%d",info.curr_ratio,info.finish_time);
			}
			else 
			{
				if (bid_info.count(last_hour))
				{
					BID_INFO &last_info = bid_info[last_hour];
					va_cout("last_ratio=%lf,last_finish_time=%d",last_info.curr_ratio,last_info.finish_time);
					info.curr_ratio = last_info.curr_ratio;
					if (info.curr_ratio > INIT_COMP_RATIO) // 上一个小时没有设置过比例，按照创意的原始价格出价
					{
						info.curr_ratio = 0;
					}
					else if (last_info.finish_time == INIT_FINISH_TIME)   // 没有结束时间，升价
					{

						info.curr_ratio = min(last_info.curr_ratio + DISCOUNT, redis_cashe->grsmartbidinfo[groupid].maxratio - 1);

					}
					else if (last_info.finish_time <= redis_cashe->grsmartbidinfo[groupid].interval) // 降价
					{

						info.curr_ratio = max(last_info.curr_ratio - DISCOUNT, redis_cashe->grsmartbidinfo[groupid].minratio - 1);
					}
				}
				else
					info.curr_ratio = 0;

				write_bid_info = group_bid_info[groupid];
				flag = true;
			}


			creative.price = creative.price *(1 + info.curr_ratio);
			cflog(fctx->logid, LOGDEBUG, "%s,%s/%d,groupid:%s,mapid:%s,curr_ratio:%lf",
				bid, __FUNCTION__, __LINE__, groupid.c_str(), creative.mapid.c_str(), info.curr_ratio);
		}
	}

unlock:
	pthread_mutex_unlock(&fctx->bid_info_mutex);

	if (flag)  // have fix price
	{
		write_smart_ratio(fctx, groupid, write_bid_info, tm.tm_mday);
	}
	record_cost(fctx->logid, fctx->is_print_time, bid, "adjust_ordinary_bid_price", ts1);
	return true;
}

static bool filter_banner_battr(uint64_t logid, IN const set<uint8_t> &battr, IN const set<uint8_t> &attr)
{
	bool bpass = true;

	if ((battr.size() != 0) && (attr.size() != 0))
	{
		set<uint8_t>::iterator s_attr_it = attr.begin();
		for(s_attr_it; s_attr_it != attr.end(); s_attr_it++)
		{
			set<uint8_t>::iterator s_battr_it = battr.find(*s_attr_it);

			if(s_battr_it != battr.end())
			{
				bpass = false;
				break;
			}
		}	
	}

	return bpass;
}

static inline int filter_creative_by_btype_battr_size_price_cb(
	IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, INOUT COM_REQUEST &crequest, INOUT CREATIVEOBJECT &creative, 
	IN bool whilelist, IN bool is_blist, IN CALLBACK_CHECK_PRICE callback_check_price, IN bool is_print_time)
{
	timespec ts1;
	getTime(&ts1);
	uint64_t logid = fctx->logid;
	int j = 0;

	int errcode = E_SUCCESS;
	char *id = (char*)crequest.id.c_str();

	cflog(logid, LOGINFO, "%s, %s/%d, enter", 
		id, __FUNCTION__, __LINE__);

	if (!creative.deeplink.empty() && !crequest.support_deep_link)
	{
		// deeplink型创意只投deeplink流量
		writeLog(logid, LOGINFO, "mapid %s support deeplink! req not deeplink", creative.mapid.c_str());
		return E_FILTER_DEEPLINK;
	}

	const COM_IMPRESSIONOBJECT &cimp = crequest.imp[0];
	switch (cimp.type)
	{
	case IMP_BANNER:
		{
			const COM_BANNEROBJECT &cbanner = cimp.banner;

			cflog(logid, LOGINFO, "%s, %s/%d, Comparing... groupid:%s, mapid:%s, cbanner.w:%d, cbanner.h:%d, creative.w:%d, creative.h:%d", 
				id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str(), cbanner.w, cbanner.h, creative.w, creative.h);

			{
				cflog(logid, LOGINFO, "%s, %s/%d, match! w:%d, h:%d", 
					id, __FUNCTION__, __LINE__, creative.w, creative.h);

				set<uint8_t>::iterator s_btype_it = cbanner.btype.find(creative.type);
				if(s_btype_it == cbanner.btype.end())
				{ 
					bool bpass = filter_banner_battr(logid, cbanner.battr, creative.attr);
					if (!bpass)
					{
						cflog(logid, LOGWARNING, "%s, %s/%d, NOT match attr! groupid:%s, mapid:%s, creative.attr:%d in req.battr", 
							id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str(), creative.type);

						errcode = E_FILTER_IMP_BANNER_BATTR;
						goto exit;
					}	
				}	
				else
				{
					cflog(logid, LOGWARNING, "%s, %s/%d, NOT match type! groupid:%s, mapid:%s, creative.type:%d in req.btype", 
						id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str(), creative.type);

					errcode = E_FILTER_IMP_BANNER_BTYPE;
					goto exit;
				}
			}
		}
		break;

	case IMP_VIDEO:
		{
			/*const COM_VIDEOOBJECT &cvideo = cimp.video;

			if (creative.ftype != ADFTYPE_VIDEO_MP4)
			{
			errcode = E_FILTER_IMP_VIDEO_FTYPE;
			goto exit;
			}	*/
		}
		break;
	case IMP_NATIVE:
		{
			const COM_NATIVE_REQUEST_OBJECT &native = cimp.native;

			if (native.bctype.find(creative.ctype) == native.bctype.end())
			{ 
				for (int i = 0; i < native.assets.size(); i++)
				{
					const COM_ASSET_REQUEST_OBJECT &asset = native.assets[i];

					if (asset.required == 0)
						continue;

					//creative 的合法性由缓存模块保证

					if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						if (asset.title.len < creative.title.length())
						{
							errcode = E_FILTER_IMP_NATIVE_ASSET_TITLE_LENGTH;
							goto exit;
						}
					}
					else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						const COM_IMAGE_REQUEST_OBJECT &img = asset.img;

						if (img.type == ASSET_IMAGETYPE_ICON) 
						{
							if((native.img_num == 0) && (creative.imgs.size() > 0))
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_NUM;
								goto exit;
							}
							if (!((img.w == creative.icon.w) && (img.h == creative.icon.h)))
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_SIZE;
								goto exit;
							}

							if ((img.mimes.size() > 0) && (img.mimes.count(creative.icon.ftype) == 0))
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_FTYPE;
								goto exit;
							}

							if (creative.icon.sourceurl == "")
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_URL;
								goto exit;
							}
						}
						else if (img.type == ASSET_IMAGETYPE_MAIN)
						{
							if(native.img_num != creative.imgs.size())
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_NUM;
								goto exit;
							}

							if (!((img.w == creative.imgs[j].w ) && (img.h == creative.imgs[j].h)))
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_SIZE;
								goto exit;
							}

							if ((img.mimes.size() > 0) && (img.mimes.count(creative.imgs[j].ftype) == 0))
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_FTYPE;
								goto exit;
							}

							if (creative.imgs[j].sourceurl == "")
							{
								errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_URL;
								goto exit;
							}
							j++;
						}
						else
						{	
							errcode = E_FILTER_IMP_NATIVE_ASSET_IMAGE_TYPE;
							goto exit;
						}
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						const COM_DATA_REQUEST_OBJECT &data = asset.data;

						if ((data.type == ASSET_DATATYPE_DESC) && (data.len < creative.description.length()))
						{
							errcode = E_FILTER_IMP_NATIVE_ASSET_DATA_DESC_LENGTH;
							goto exit;
						}
						else if ((data.type == ASSET_DATATYPE_RATING) && (creative.rating > 5))
						{
							errcode = E_FILTER_IMP_NATIVE_ASSET_DATA_RATING_VALUE;
							goto exit;
						}
						else if(data.type == ASSET_DATATYPE_CTATEXT && creative.ctatext == "")
						{
							errcode = E_FILTER_IMP_NATIVE_ASSET_DATA_CTATEXT;
							goto exit;
						}
					}
					else
					{
						errcode = E_FILTER_IMP_NATIVE_ASSET_TYPE;
						goto exit;
					}
				}
			}	
			else
			{
				cflog(logid, LOGWARNING, "%s, %s/%d, NOT match ctype! groupid:%s, mapid:%s, creative.ctype:%d in req.cbtype", 
					id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str(), creative.ctype);

				errcode = E_FILTER_IMP_NATIVE_BCTYPE;
				goto exit;
			}
		}
		break;

	default:
		{
			errcode = E_FILTER_IMP_UNSUPPORTED;
			goto exit;
		}
		break;
	}

	if (crequest.at == BID_PMP && crequest.imp[0].dealprice.size() > 0)
	{
		if (crequest.imp[0].dealprice.count(creative.dealid))
		{
			crequest.imp[0].bidfloor = crequest.imp[0].dealprice[creative.dealid];
		}
	}

	if (crequest.imp[0].fprice.size() > 0)
	{
		if(crequest.imp[0].fprice.count(creative.advcat))
		{
			crequest.imp[0].bidfloor = crequest.imp[0].fprice[creative.advcat];
		}
	}
	if (whilelist && !is_blist)
	{
		if (!adjust_wlist_bid_price(redis_cashe, crequest, creative))
		{
			cflog(logid, LOGWARNING, "%s, %s/%d, adjust_wlist_bid_price failed! bidfloor:%f, creative.price:%f, creative.groupid:%s", 
				id, __FUNCTION__, __LINE__, crequest.imp[0].bidfloor, creative.price, creative.groupid.c_str());

			errcode = E_FILTER_PRICE_WLIST;   
			goto exit;
		}
	}
	else
	{
		if (redis_cashe->grsmartbidinfo.count(creative.groupid) && redis_cashe->grsmartbidinfo[creative.groupid].flag == 1) // 调整价格
		{
			// 普通项目根据前一个小时修改价格
			adjust_ordinary_bid_price(fctx, redis_cashe, id, creative);
		}
		cflog(logid, LOGDEBUG, "%s,%s/%d,smart,groupid:%s,mapid:%s,price:%lf",
			id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str(), creative.price);
	}

	if (((callback_check_price != NULL) && callback_check_price(crequest, redis_cashe->adxtemplate.ratio, creative.price ,creative.advcat)) || 
		(callback_check_price == NULL))
	{
		if (callback_check_price != NULL)
		{
			cflog(logid, LOGINFO, "%s, %s/%d, callback_check_price return true! groupid:%s, mapid:%s", 
				id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str());
		}
	}
	else
	{
		cflog(logid, LOGWARNING, "%s, %s/%d, ignore this creative, filter_cbidobject_cb return false! groupid:%s, mapid:%s", 
			id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str());

		errcode = E_FILTER_PRICE_CALLBACK;   // 11-13 与上面price错误码区分
		goto exit;
	}

exit:

	cflog(logid, LOGINFO, "%s, leave %s/%d, errcode:0x%x", id, __FUNCTION__, __LINE__, errcode);

	record_cost(logid, is_print_time, id, string(string("filter_creative_by_btype_battr_size_price_cb inside, id: ") + crequest.id).c_str(), ts1);

	return errcode;
}

bool find_set_cat(IN uint64_t logid, IN const set<uint32_t> &cat_fix, IN const set<uint32_t> &cat)
{
	bool bfind = false;

	if ((cat_fix.size() != 0) && (cat.size() != 0))
	{
		set<uint32_t>::iterator s_cat_it = cat.begin();
		for(; s_cat_it != cat.end(); s_cat_it++)
		{
			uint32_t cat = *s_cat_it;

			set<uint32_t>::iterator s_bcat_it;

			s_bcat_it = cat_fix.find(cat & 0xFFFFFF);
			if (s_bcat_it != cat_fix.end())
			{
				//	va_cout("find cat: 0x%x  & 0xFFFFFF", cat);
				bfind = true;
				break;
			}

			s_bcat_it = cat_fix.find(cat & 0xFFFF00);
			if (s_bcat_it != cat_fix.end())
			{
				//	va_cout("find cat: 0x%x  & 0xFFFF00", cat);
				bfind = true;
				break;
			}

			s_bcat_it = cat_fix.find(cat & 0xFF0000);
			if (s_bcat_it != cat_fix.end())
			{
				//	va_cout("find cat: 0x%x  & 0xFF0000", cat);
				bfind = true;
				break;
			}
		}	
	}

	return bfind;
}

static int filter_scene_target(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN const COM_REQUEST &crequest, 
							   IN const char *szgroupid, SCENE_TARGET &scene_target)
{
	uint64_t logid = fctx->logid;

	char *id = (char *)crequest.id.c_str();

	int errcode = E_FILTER_TARGET_SCENE;

	const int flag = scene_target.flag;

	cflog(logid, LOGINFO, "%s, groupid:%s, %s, flag:0x%x", id, szgroupid, __FUNCTION__, flag);  

	if ((fctx == NULL) || (redis_cashe == NULL))
	{
		cflog(logid, LOGWARNING, "%s, filter_scene_target(fctx:0x%x, redis_cashe:0x%x) param failed!", id, fctx, redis_cashe);
		goto exit;
	}

	if (flag == TARGET_ALLOW_ALL)
	{
		errcode = E_SUCCESS;
		goto exit;
	}
	else if (flag == TARGET_SCENE_DISABLE_ALL)
	{
		goto exit;
	}
	else
	{
		//scene
		if (flag & TARGET_SCENE_MASK)
		{
			if(strlen(crequest.device.locid) > 0)
			{
				string locid = string(crequest.device.locid, scene_target.length);
				cflog(logid, LOGINFO, "locid :%s", locid.c_str());
				set<string>::iterator loc_it = scene_target.loc_set.find(locid);

				if (loc_it != scene_target.loc_set.end())
				{
					if (flag & TARGET_SCENE_WHITELIST)
					{
						cflog(logid, LOGWARNING, "%s, groupid:%s, device.locid:%s in locid whitelist.", 
							id, szgroupid, crequest.device.locid);
						errcode = E_SUCCESS;
						goto exit;
					}
				}
				else
				{
					if (flag & TARGET_SCENE_BLACKLIST)
					{
						cflog(logid, LOGWARNING, "%s, groupid:%s, device.locid:%s in locid whitelist.", 
							id, szgroupid, crequest.device.locid);
						errcode = E_SUCCESS;
						goto exit;
					}
				}
			}
		}
	}

exit:
	return errcode;
}

static int filter_app_target(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN const COM_REQUEST &crequest, IN const char *szgroupid, 
							 APP_TARGET &app_target)
{
	uint64_t logid = fctx->logid;

	char *id = (char *)crequest.id.c_str();

	int errcode = E_SUCCESS;

	int flag = ((app_target.id.flag | (app_target.cat.flag << 2 )) & TARGET_APP_MASK);

	if ((fctx == NULL) || (redis_cashe == NULL))
	{
		cflog(logid, LOGWARNING, "%s, filter_app_target(fctx:0x%x, redis_cashe:0x%x) param failed!", id, fctx, redis_cashe);
		goto exit;
	}

	cflog(logid, LOGINFO, "%s, groupid:%s, %s, flag:0x%x", id, szgroupid, __FUNCTION__, flag);

	if (flag == TARGET_ALLOW_ALL)
	{
		errcode = E_SUCCESS;
		goto exit;
	}
	else
	{
		if (flag == TARGET_APP_ID_MASK)
			flag = TARGET_APP_ID_BLACKLIST;
		else if (flag == TARGET_APP_CAT_MASK)
			flag = TARGET_APP_CAT_BLACKLIST;

		if (flag & TARGET_APP_ID_MASK)
		{
			if (flag & TARGET_APP_ID_BLACKLIST)
			{
				set<string> &s_id_blist = app_target.id.blist;
				set<string>::iterator s_id_it = s_id_blist.find(crequest.app.id);
				if (s_id_it != s_id_blist.end()) // 在名单中
				{
					errcode = E_FILTER_TARGET_APP_ID;
					goto exit;
				}	

				if (!(flag & TARGET_APP_CAT_MASK))//there is only id blist or wlist
				{
					errcode = E_SUCCESS;
					goto exit;
				}
			}

			if (flag & TARGET_APP_ID_WHITELIST)
			{
				set<string> &s_id_wlist = app_target.id.wlist;
				set<string>::iterator s_id_it = s_id_wlist.find(crequest.app.id);
				if (s_id_it != s_id_wlist.end()) // 在名单中
				{
					errcode = E_SUCCESS;
					goto exit;
				}	

				if (!(flag & TARGET_APP_CAT_MASK))//there is only id blist or wlist
				{
					errcode = E_FILTER_TARGET_APP_ID;
					goto exit;
				}
			}	
		}

		if (flag & TARGET_APP_CAT_MASK)
		{
			if (flag & TARGET_APP_CAT_BLACKLIST)
			{
				set<uint32_t> &s_cat_blist = app_target.cat.blist;
				if (find_set_cat(logid, s_cat_blist, crequest.app.cat) || find_set_cat(logid, crequest.app.cat, s_cat_blist))
				{// 在名单中
					dump_set_cat(logid, LOGWARNING, "s_cat_blist", s_cat_blist);
					dump_set_cat(logid, LOGWARNING, "crequest.app.cat", crequest.app.cat);

					errcode = E_FILTER_TARGET_APP_CAT;
					goto exit;
				}

				if (!(flag & TARGET_APP_ID_MASK))
				{
					errcode = E_SUCCESS;
					goto exit;
				}
			}

			if (flag & TARGET_APP_CAT_WHITELIST)
			{
				set<uint32_t> &s_cat_wlist = app_target.cat.wlist;
				if (find_set_cat(logid, s_cat_wlist, crequest.app.cat) || find_set_cat(logid, crequest.app.cat, s_cat_wlist))
				{// 在名单中
					errcode = E_SUCCESS;
					goto exit;
				}

				dump_set_cat(logid, LOGWARNING, "s_cat_wlist", s_cat_wlist);
				dump_set_cat(logid, LOGWARNING, "crequest.app.cat", crequest.app.cat);

				errcode = E_FILTER_TARGET_APP_CAT;
				goto exit;
			}
			else
			{
				errcode = E_SUCCESS;
				goto exit;
			}
		}
	}

exit:

	return errcode;
}

static int filter_device_target(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN const COM_REQUEST &crequest, IN const char *szgroupid, 
								DEVICE_TARGET &device_target)
{
	uint64_t logid = fctx->logid;

	char *id = (char *)crequest.id.c_str();

	int errcode = E_SUCCESS;

	const int flag = device_target.flag;

	int location = crequest.device.location - CHINAREGIONCODE;

	cflog(logid, LOGINFO, "%s, groupid:%s, %s, flag:0x%x", id, szgroupid, __FUNCTION__, flag);  

	bool bgeo = false, bcat = false, bconnectiontype = false, bos = false, bcarrier = false, bdevicetype = false, bmake = false;
	if ((fctx == NULL) || (redis_cashe == NULL))
	{
		cflog(logid, LOGWARNING, "%s, filter_device_target(fctx:0x%x, redis_cashe:0x%x) param failed!", id, fctx, redis_cashe);

		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if (flag == TARGET_ALLOW_ALL)
	{
		errcode = E_SUCCESS;
	}
	else if (flag == TARGET_DEVICE_DISABLE_ALL)
	{
		errcode = E_FILTER_TARGET_DEVICE;
	}
	else
	{
		//1. regioncode
		if (flag & TARGET_DEVICE_REGIONCODE_MASK)
		{
			if (location != 0)
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, device_target.regioncode.size():%d.", id, szgroupid, device_target.regioncode.size());

				set<uint32_t>::iterator regioncode_it = device_target.regioncode.find(location);

				if (regioncode_it != device_target.regioncode.end())
				{
					if(flag & TARGET_DEVICE_REGIONCODE_WHITELIST)
					{
						bgeo = true;
					}
				}
			}

			if (!bgeo)
			{
				cflog(logid, LOGWARNING, "%s, groupid:%s, ip:%s -> location:%d, %s in geo whitelist.", 
					id, szgroupid, crequest.device.ip.c_str(), location, (bgeo ? "  " : "not"));

				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.regioncode ...", id, szgroupid);
				dump_set_regioncode(logid, LOGWARNING, id, device_target.regioncode);
				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.regioncode over!", id, szgroupid);

				errcode = E_FILTER_TARGET_DEVICE_REGIONCODE;
				goto exit;
			}
			else
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, ip:%s -> location:%d, %s in geo whitelist.", 
					id, szgroupid, crequest.device.ip.c_str(), location, (bgeo ? "  " : "not"));
			}
		}
		else
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, allow all geo.", id, szgroupid);
			bgeo = true;
		}

		//2. connectiontype
		if (flag & TARGET_DEVICE_CONNECTIONTYPE_MASK)
		{
			set<uint8_t>::iterator connectiontype_it = device_target.connectiontype.find(crequest.device.connectiontype);

			if(connectiontype_it != device_target.connectiontype.end())
			{
				if(flag & TARGET_DEVICE_CONNECTIONTYPE_WHITELIST)
				{
					bconnectiontype = true;
				}
			}		

			if (!bconnectiontype)
			{
				cflog(logid, LOGWARNING, "%s, groupid:%s, connectiontype:%d, %s in connectiontype whitelist.", 
					id, szgroupid, crequest.device.connectiontype, (bconnectiontype ? "  " : "not"));

				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.connectiontype ...", id, szgroupid);
				dump_set_connectiontype(logid, LOGWARNING, id, device_target.connectiontype);
				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.connectiontype over!", id, szgroupid);

				errcode = E_FILTER_TARGET_DEVICE_CONNECTIONTYPE;
				goto exit;
			}
			else
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, connectiontype:%d, %s in connectiontype whitelist.", 
					id, szgroupid, crequest.device.connectiontype, (bconnectiontype ? "  " : "not"));
			}
		}
		else
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, allow all connectiontype.", id, szgroupid);
			bconnectiontype = true;
		}

		//3. os
		if (flag & TARGET_DEVICE_OS_MASK)
		{
			set<uint8_t>::iterator os_it = device_target.os.find(crequest.device.os);

			if (os_it != device_target.os.end())
			{
				if (flag & TARGET_DEVICE_OS_WHITELIST)
				{
					bos = true;
				}
			}

			if (!bos)
			{
				cflog(logid, LOGWARNING, "%s, groupid:%s, device.os:%d, %s in os whitelist.", 
					id, szgroupid, crequest.device.os, (bos ? "  " : "not"));

				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.os ...", id, szgroupid);
				dump_set_os(logid, LOGWARNING, id, device_target.os);
				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.os over!", id, szgroupid);

				errcode = E_FILTER_TARGET_DEVICE_OS;
				goto exit;
			}
			else
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, device.os:%d, %s in os whitelist.", 
					id, szgroupid, crequest.device.os, (bos ? "  " : "not"));
			}
		}
		else
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, allow all os.", id, szgroupid);
			bos = true;
		}

		//4. carrier
		if (flag & TARGET_DEVICE_CARRIER_MASK)
		{
			set<uint8_t>::iterator carrier_it = device_target.carrier.find(crequest.device.carrier);

			if (carrier_it != device_target.carrier.end())
			{
				if (flag & TARGET_DEVICE_CARRIER_WHITELIST)
				{
					bcarrier = true;
				}
			}

			if (!bcarrier)
			{
				cflog(logid, LOGWARNING, "%s, groupid:%s, device.carrier:%d, %s in carrier whitelist.", 
					id, szgroupid, crequest.device.carrier, (bcarrier ? "  " : "not"));

				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.carrier ...", id, szgroupid);
				dump_set_carrier(logid, LOGWARNING, id, device_target.carrier);
				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.carrier over!", id, szgroupid);

				errcode = E_FILTER_TARGET_DEVICE_CARRIER;
				goto exit;
			}
			else
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, device.carrier:%d, %s in carrier whitelist.", 
					id, szgroupid, crequest.device.carrier, (bcarrier ? "  " : "not"));
			}
		}
		else
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, allow all carrier.", id, szgroupid);
			bcarrier = true;
		}

		//5. devicetype
		if (flag & TARGET_DEVICE_DEVICETYPE_MASK)
		{
			set<uint8_t>::iterator devicetype_it = device_target.devicetype.find(crequest.device.devicetype);

			if (devicetype_it != device_target.devicetype.end())
			{
				if (flag & TARGET_DEVICE_DEVICETYPE_WHITELIST)
				{
					bdevicetype = true;
				}
			}

			if (!bdevicetype)
			{
				cflog(logid, LOGWARNING, "%s, groupid:%s, device.devicetype:%d, %s in devicetype whitelist.", 
					id, szgroupid, crequest.device.devicetype, (bdevicetype ? "  " : "not"));

				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.devicetype ...", id, szgroupid);
				dump_set_devicetype(logid, LOGWARNING, id, device_target.devicetype);
				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.devicetype over!", id, szgroupid);

				errcode = E_FILTER_TARGET_DEVICE_DEVICETYPE;
				goto exit;
			}
			else
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, device.devicetype:%d, %s in devicetype whitelist.", 
					id, szgroupid, crequest.device.devicetype, (bdevicetype ? "  " : "not"));
			}
		}
		else
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, allow all devicetype.", id, szgroupid);
			bdevicetype = true;
		}

		//6. make
		if (flag & TARGET_DEVICE_MAKE_MASK)
		{
			set<uint16_t>::iterator make_it = device_target.make.find(crequest.device.make);

			if (make_it != device_target.make.end())
			{
				if (flag & TARGET_DEVICE_MAKE_WHITELIST)
				{
					bmake = true;
				}
			}

			if (!bmake)
			{
				cflog(logid, LOGWARNING, "%s, groupid:%s, device.make:%d, %s in make whitelist.", 
					id, szgroupid, crequest.device.make, (bmake ? "  " : "not"));

				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.make ...", id, szgroupid);
				dump_set_make(logid, LOGWARNING, id, device_target.make);
				cflog(logid, LOGWARNING, "%s, groupid:%s, dump device_target.make over!", id, szgroupid);

				errcode = E_FILTER_TARGET_DEVICE_MAKE;
				goto exit;
			}
			else
			{
				cflog(logid, LOGINFO, "%s, groupid:%s, device.make:%d, %s in make whitelist.", 
					id, szgroupid, crequest.device.make, (bmake ? "  " : "not"));
			}
		}
		else
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, allow all make.", id, szgroupid);
			bmake = true;
		}
	}

exit:

	if (errcode != E_SUCCESS)
	{
		cflog(logid, LOGWARNING, "%s, groupid:%s, device_flag:0x%0.12x, bgeo:%d, bconnectiontype:%d, bos:%d, bcarrier:%d, bdevicetype:%d, bmake:%d", 
			id, szgroupid, flag, bgeo, bconnectiontype, bos, bcarrier, bdevicetype, bmake);
	}

	return errcode;
}

static bool filter_request_bcat(IN uint64_t logid, IN const set<uint32_t> &bcat, IN const set<uint32_t> &cat)
{
	return !(find_set_cat(logid, bcat, cat) || find_set_cat(logid, cat, bcat));
}

static void compute_ctr(IN FILTERCONTEXT *fctx, IN const COM_REQUEST &crequest, IN double price, IN string mapid)
{
	uint64_t logid = fctx->logid;
	double intercept = 0;
	double atype = 0;
	double bidf = 0;
	double instl = 0;
	double isp = 0;
	double nt = 0;
	double dt = 0;
	double size = 0;
	double mfr = 0;
	double period = 0;
	double z = 0;
	double p = 0;
	if (crequest.imp[0].ext.instl == INSTL_FULL)
		return;
	if (crequest.imp[0].ext.instl == INSTL_INSERT)
		instl = 2.00221;

	intercept = -0.52505;
	switch(fctx->adx)
	{
	case ADX_TANX:
		atype = 0;
		break;
	case ADX_INMOBI:
		atype = -0.11418;
		break;
	case ADX_ZPLAY:
		atype = -0.18947;
		break;
	case ADX_BAIDU:
		atype = -0.12411;
		break;
	case ADX_IQIYI:
		atype = -1.8703;
		break;
	case ADX_IFLYTEK:
		atype = -0.91988;
		break;
	case ADX_ADVIEW:
		atype = -0.70354;
		break;
	default:
		atype = 0.30347;
		break;
	}

	bidf = 392.95318 * crequest.imp[0].bidfloor / 1000;

	switch(crequest.device.carrier)
	{
	case CARRIER_CHINAMOBILE:
		isp = -0.16647;
		break;
	case CARRIER_CHINAUNICOM:
		isp = -0.12564;
		break;
	case CARRIER_CHINATELECOM:
		isp = -0.14862;
		break;
	default:
		isp = 0;
		break;
	}
	switch(crequest.device.make)
	{
	case 1:  //apple
		mfr = 0.53502;
		break;
	case 2:  //samsung
		mfr = 0.60559;
		break;
	case 3:  //xiaomi
		mfr = 0.24409;
		break;
	case 4:  //huawei
		mfr = -0.10958;
		break;
	case 5:  //meizu
		mfr = 0.21421;
		break;
	case 8:     //vivo
		mfr = 0.07182;
		break;
	case 9:     //coolpad
		mfr = 0.40671;
		break;
	case 12:     //oppo
		mfr = 0;
		break;
	case 36:     //jinli
		mfr = 0.07336;
		break;
	default:
		mfr = 0.45174;
		break;
	}
	switch(crequest.device.connectiontype)
	{
	case CONNECTION_WIFI:
		nt = 0.21554;
		break;
	case CONNECTION_CELLULAR_UNKNOWN:
		nt = -0.69044;
		break;
	case CONNECTION_CELLULAR_2G:
		nt = -0.31604;
		break;
	case CONNECTION_CELLULAR_3G:
		nt = -0.26452;
		break;
	case CONNECTION_CELLULAR_4G:
		nt = -0.23705;
		break;
	default:
		nt = 0;
		break;
	}
	if (crequest.device.devicetype == DEVICE_TABLET)
		dt = -0.61271;
	if(crequest.imp[0].banner.w == 600 && crequest.imp[0].banner.h == 500)
	{
		size = -0.59487;
	}
	else if (crequest.imp[0].banner.w == 640 && crequest.imp[0].banner.h == 100)
	{
		size = 0.2078;
	}
	else if (crequest.imp[0].banner.w == 320 && crequest.imp[0].banner.h == 50)
	{
		size = 0;
	}
	else 
	{
		size = 0.20275;
	}

	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	if (tm.tm_hour >= 6 && tm.tm_hour < 13)
	{
		period = 0;
	}
	else if (tm.tm_hour >= 13 && tm.tm_hour < 19)
	{
		period = 0.16846;
	}
	else if (tm.tm_hour >= 19 || tm.tm_hour < 6)
	{
		period = 0.22382;
	}

	z = intercept + atype + bidf + instl + isp + nt + dt + size + mfr + period;
	p = exp(z) / (1 + exp(z));

	cflog(logid, LOGDEBUG, "compute ctr,%s,%s,%d,%s,%d,%lf,%lf", 
		crequest.id.c_str(), crequest.device.deviceid.c_str(), crequest.device.deviceidtype, mapid.c_str(), fctx->adx, price, p);
}

// 若流量要求deeplink，则优先选择deeplink型创意
static void select_deeplink(IN int req_deeplink, INOUT vector<CREATIVEOBJECT> &v_creative, 
	INOUT vector<int> &index, OUT map<string, ERRCODE_GROUP> &m_err_group)
{
	if (!req_deeplink || v_creative.size() < 2 || index.size() < 2)
	{
		return;
	}

	vector<int> support;
	vector<const CREATIVEOBJECT*> del_creatives;
	for (size_t i = 0; i < index.size(); i++)
	{
		if (!v_creative[index[i]].deeplink.empty())
		{
			support.push_back(index[i]);
		}
		else
		{
			del_creatives.push_back(&v_creative[index[i]]);
		}
	}

	if (support.empty()){
		return; // 几个创意都不支持
	}

	for (size_t i = 0; i < del_creatives.size(); i++)
	{
		m_err_group[del_creatives[i]->groupid].m_err_creative[del_creatives[i]->mapid] = E_FILTER_DEEPLINK;
	}

	index = support;
}

static bool filter_creative(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN redisContext *redis_ur, 
						   IN redisContext *redis_user_advcat, INOUT COM_REQUEST &crequest,
						   IN CALLBACK_CHECK_PRICE callback_check_price, IN CALLBACK_CHECK_EXT callback_check_ext,
						   IN const vector<pair<string, GROUPINFO> > &v_p_groupinfo, const map<string, bool> &group_has_blist,
						   IN bool whilelist, INOUT USER_FREQUENCY_COUNT &ur, 
						   OUT COM_BIDOBJECT &dst_cbid, INOUT map<string, ERRCODE_GROUP> &m_err_group)
{
	bool ret = false;
	const char *sequence = (whilelist  ? "first" : "second");

	const char *id = crequest.id.c_str();
	uint64_t logid = fctx->logid;
	map<double, vector<CREATIVEOBJECT>, greater<double> > m_v_creative;//价格降序排列bid vector，每一个bid vector具有相同的价格
	map<double, vector<CREATIVEOBJECT>, greater<double> >::iterator pos, pos2;
	timespec ts1;
	vector<int> index;

	cflog(logid, LOGINFO, "%s, enter %s/%d....%s", id, __FUNCTION__, __LINE__, sequence);

	getTime(&ts1);

	for (int i = 0; i < v_p_groupinfo.size(); i++)
	{
		const string &groupid = v_p_groupinfo[i].first;
		//	GROUPINFO &groupinfo = v_p_groupinfo[i].second;
		map<string, vector<string> > &m_mapid = redis_cashe->groupid;
		map<string, vector<string> >::iterator m_mapid_it = m_mapid.find(groupid);
		if (m_mapid_it == m_mapid.end())
		{
			continue;//cache data error!!!
		}

		vector<string> &v_mapid = m_mapid_it->second;

		cflog(logid, LOGINFO, "%s, %s/%d, v_groupid[%d]:%s, and its v_mapid.size:%d",
			id, __FUNCTION__, __LINE__, i, groupid.c_str(), v_mapid.size());

		map<string, CREATIVEOBJECT> &m_creative = redis_cashe->creativeinfo;   
		for (int j = 0; j < v_mapid.size(); j++)
		{
			map<string, CREATIVEOBJECT>::iterator m_creative_it = m_creative.find(v_mapid[j]);

			if(m_creative_it == redis_cashe->creativeinfo.end())
			{
				continue;//cache data error!!!
			}

			CREATIVEOBJECT &creative = m_creative_it->second;
			const COM_IMPRESSIONOBJECT &cimp = crequest.imp[0];
			if (cimp.type == IMP_BANNER)
			{
				if (creative.type != ADTYPE_IMAGE)
				{
					m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_IMP_BANNER_TYPE));
					continue;
				}

				const COM_BANNEROBJECT &cbanner = cimp.banner;
				if(!((cbanner.w == creative.w) && (cbanner.h == creative.h)))
				{
					m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_IMP_BANNER_SIZE));
					continue;
				}

			}
			else if (cimp.type == IMP_NATIVE)
			{
				if (creative.type != ADTYPE_FEEDS)
				{
					m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_IMP_NATIVE_TYPE));
					continue;
				}
			}
			else if (cimp.type == IMP_VIDEO)
			{
				if (creative.type != ADTYPE_VIDEO)
				{
					m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_IMP_VIDEO_TYPE));
					continue;
				}
				if(cimp.video.is_limit_size)
				{
					const COM_VIDEOOBJECT &cvideo = cimp.video;
					if(!((cvideo.w == creative.w) && (cvideo.h == creative.h)))
					{
						m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_IMP_VIDEO_SIZE));
						continue;
					}
				}

				if((cimp.video.mimes.size() >0) && (cimp.video.mimes.count(creative.ftype) == 0))
				{
					m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_IMP_VIDEO_FTYPE));
					continue;
				}
			}

			if (callback_check_ext != NULL)
			{
				if (!callback_check_ext(crequest, creative.ext))
				{
					cflog(logid, LOGWARNING, "%s, %s/%d, ignore this creative, callback_check_exts failed! groupid:%s, mapid:%s", 
						id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str());

					m_err_group[groupid].m_err_creative.insert(make_pair(v_mapid[j], E_FILTER_EXTS)); // 修改错误码 11-13
					continue;
				}

				cflog(logid, LOGINFO, "%s, %s/%d, accept this creative, callback_check_exts success! groupid:%s, mapid:%s", 
					id, __FUNCTION__, __LINE__, creative.groupid.c_str(), creative.mapid.c_str());
			}

			m_v_creative[creative.price].push_back(creative);
		}
	}
	record_cost(logid, fctx->is_print_time, id, "filter_merge_group_bids", ts1);

	cflog(logid, (m_v_creative.size() ? LOGINFO : LOGWARNING), "%s, %s/%d, after filter_merge_group_bids(%s), m_v_creative.size:%d", 
		id, __FUNCTION__, __LINE__, sequence, m_v_creative.size());

	if (m_v_creative.size() == 0)
	{
		dump_vector_pair_groupinfo(logid, LOGWARNING, id, v_p_groupinfo);
		goto exit;
	}

	for (pos = m_v_creative.begin(); pos != m_v_creative.end(); pos++)
	{	
		vector<CREATIVEOBJECT> &v_creative = pos->second;

		for(int count = 0; count < v_creative.size(); ++count)
		{
			CREATIVEOBJECT &creative = v_creative[count];
			bool has_blist = group_has_blist.find(creative.groupid) != group_has_blist.end();

			cflog(logid, LOGINFO, "%s, %s/%d, get a v_creative[%d](groupid:%s, mapid:%s)", 
				id, __FUNCTION__, __LINE__, count, creative.groupid.c_str(), creative.mapid.c_str());

			getTime(&ts1);
			int check_errcode = filter_creative_by_btype_battr_size_price_cb(fctx, redis_cashe, crequest, creative, 
				whilelist, has_blist, callback_check_price, fctx->is_print_time);
			record_cost(logid, fctx->is_print_time, id, 
				string(string("filter_creative_by_btype_battr_size_price_cb outside, id: ") + crequest.id).c_str(), ts1);

			if (check_errcode != E_SUCCESS)
			{
				cflog(logid, LOGINFO, "%s, %s/%d, v_creative[%d](groupid:%s, mapid:%s), filter_creative_by_btype_battr_size_price_cb failed, check_errcode:0x%x", 
					id, __FUNCTION__, __LINE__, count, creative.groupid.c_str(), creative.mapid.c_str(), check_errcode);

				m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, check_errcode));  // 11-13 这里插入的错误码不对，应该插入errcode

				continue;
			}

			map<string, FREQUENCYCAPPINGRESTRICTION>::iterator m_group_fcr_it = redis_cashe->fc.find(creative.groupid);
			if ((m_group_fcr_it != redis_cashe->fc.end()) && (m_group_fcr_it->second.fru.type != USER_FREQUENCY_INFINITY))
			{
				if (ur.userid == "")//如果没有读取过ur，则读取之
				{
					getTime(&ts1);
					int b_exist = readUserRestriction(redis_ur, logid, crequest.device.deviceid, ur);
					record_cost(logid, fctx->is_print_time, id, "r_ur", ts1);

					if (b_exist != E_SUCCESS)
					{
						cflog(logid, LOGWARNING, "%s, %s/%d, readUserRestriction(deviceid:%s, deviceidtype:0x%x) failed!",
							id, __FUNCTION__, __LINE__, crequest.device.deviceid.c_str(), crequest.device.deviceidtype);

						if(b_exist == E_REDIS_CONNECT_INVALID)
						{
							cflog(logid, LOGDEBUG, "readUserRestriction redis context invalid!");
							goto exit;
						}
						else
						{
							index.push_back(count);
							continue;
						}
					}

					cflog(logid, LOGWARNING, "%s, %s/%d, readUserRestriction(deviceid:%s, deviceidtype:0x%x) success!", 
						id, __FUNCTION__, __LINE__, crequest.device.deviceid.c_str(), crequest.device.deviceidtype);

					dump_user_restriction(logid, LOGWARNING, id, ur);
				}

				FREQUENCYCAPPINGRESTRICTION &group_fcr = m_group_fcr_it->second;
				map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >::iterator p_gr = ur.user_group_frequency_count.find(group_fcr.groupid);

				if (p_gr == ur.user_group_frequency_count.end())
				{
					cflog(logid, LOGWARNING, "%s, %s/%d, v_creative[%d](groupid:%s, mapid:%s) can't find ur's restriction in this groupid:%s", 
						id, __FUNCTION__, __LINE__, count, creative.groupid.c_str(), creative.mapid.c_str(), group_fcr.groupid.c_str());
				}
				else
				{
					map<string, USER_CREATIVE_FREQUENCY_COUNT> &u_gr = p_gr->second;
					USER_CREATIVE_FREQUENCY_COUNT *cr = NULL;
					string str_mapid;
					uint32_t frequency = 0;
					if ((group_fcr.fru.type == USER_FREQUENCY_CREATIVE_2) || (group_fcr.fru.type == USER_FREQUENCY_CREATIVE_1))
					{
						str_mapid = creative.mapid;
						if (group_fcr.fru.type == USER_FREQUENCY_CREATIVE_2)
						{
							map<string, uint32_t>::iterator it = group_fcr.fru.capping.find(creative.mapid);
							if (it == group_fcr.fru.capping.end())
							{
								cflog(logid, LOGDEBUG, "%s, %s/%d, cache error!!! v_cbid[%d](mapid:%s) can't been found in group_fcr.capping", 
									id, __FUNCTION__, __LINE__, count, creative.mapid.c_str());

								continue;//用户频次中的mapid已经在cache中找不到了，可能由于group信息更改导致.几率极低，不做记录
							}

							frequency = it->second;
						}
						else
						{
							frequency = group_fcr.fru.capping.begin()->second;
						}
					}
					else if (group_fcr.fru.type == USER_FREQUENCY_PROJECT)
					{
						str_mapid = creative.groupid;
						frequency = group_fcr.fru.capping.begin()->second;
						cflog(logid, LOGWARNING, "%s, %s/%d, ctrl by group, frequency:%d", id, __FUNCTION__, __LINE__, frequency);
					}

					map<string, USER_CREATIVE_FREQUENCY_COUNT>::iterator p_cr = u_gr.find(str_mapid);

					if (p_cr != u_gr.end())
					{
						cr = &(p_cr->second);
						if ((cr != NULL) && (cr->imp >= frequency))
						{
							if((getCurrentTimeSecond() - cr->starttime) < PERIOD_TIME(group_fcr.fru.period))
							{
								cflog(logid, LOGWARNING, 
									"%s, %s/%d, deviceid:%s, v_creative[%d](groupid:%s, mapid:%s) has reached max times! imp:%d, frequency:%d!", 
									id, __FUNCTION__, __LINE__, crequest.device.deviceid.c_str(), count, creative.groupid.c_str(), 
									creative.mapid.c_str(), cr->imp, frequency);

								m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, E_FILTER_FRC));

								continue;
							}
						}
					}
				}

				index.push_back(count);
				continue;
			}
			else
			{	//某些项目不设置频次限制,随意投放
				cflog(logid, LOGINFO, "%s, %s/%d, v_creative[%d](groupid:%s, mapid:%s) don't have m_group_fcr in redis_cashe->fc", 
					id, __FUNCTION__, __LINE__, count, creative.groupid.c_str(), creative.mapid.c_str());
				index.push_back(count);
				continue;
			}
		} // end 循环遍历 v_creative，即同一价格组
		if(index.size() > 0){
			break;
		}
	} // end 循环遍历 m_v_creative

	pos2 = pos;
	if (pos != m_v_creative.end()){
		pos2++;
	}

	for (; pos2 != m_v_creative.end(); pos2++)
	{
		vector<CREATIVEOBJECT> &v_creative = pos2->second;

		for (int count = 0; count < v_creative.size(); ++count)
		{
			CREATIVEOBJECT &creative = v_creative[count];
			// 已从价高组中选了合适的创意，剩余的会被忽略
			m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, E_FILTER_PRICE_LOW));
		}
	}

exit:
	if (index.size() > 0)
	{
		vector<CREATIVEOBJECT> &v_creative = pos->second;
		
		select_deeplink(crequest.support_deep_link, v_creative, index, m_err_group);

		int cre_index = get_cre_by_catadv(fctx, crequest, redis_user_advcat, v_creative, index, m_err_group);

		va_cout("id:%s,cre_index:%d", id, cre_index);
		CREATIVEOBJECT &creative = v_creative[index[cre_index]];

		init_com_bid_object(&dst_cbid);
		dst_cbid.groupid_type = creative.groupid_type;

		for (int i = 0; i < v_p_groupinfo.size(); i++)
		{
			if (creative.groupid != v_p_groupinfo[i].first)
			{
				continue;
			}
			set_bid_object(crequest.imp[0], creative, v_p_groupinfo[i].second, crequest.is_secure, dst_cbid);
			ReplaceThirdMonitor(crequest, dst_cbid);

			if (crequest.at == BID_PMP && crequest.imp[0].dealprice.size() > 0)
			{
				if (crequest.imp[0].dealprice.count(creative.dealid))
				{
					crequest.imp[0].bidfloor = crequest.imp[0].dealprice[creative.dealid];
				}
			}

			if (crequest.imp[0].fprice.size() > 0)
			{
				if (crequest.imp[0].fprice.count(creative.advcat))
				{
					crequest.imp[0].bidfloor = crequest.imp[0].fprice[creative.advcat];
				}
			}

			if (creative.groupid == "4defff17-ae27-45ba-a83c-f05cc6589c55" &&
				crequest.imp[0].type == IMP_BANNER &&
				v_p_groupinfo[i].second.at == BID_RTB)
			{
				compute_ctr(fctx, crequest, dst_cbid.price, creative.mapid);
			}
			ret = true;
			break;
		}
	}

	return ret;
}

int get_cre_by_catadv(IN FILTERCONTEXT *fctx, INOUT COM_REQUEST &crequest, IN redisContext *redis_user_advcat,
	IN const vector<CREATIVEOBJECT> &v_creative, IN const vector<int> &index, INOUT map<string, ERRCODE_GROUP> &m_err_group)
{
	int cre_index = 0;
	const char *id = crequest.id.c_str();
	uint64_t logid = fctx->logid;
	va_cout("id:%s,index size :%d\n", id, index.size());
	set<int> advcat_cre_index;

	USER_CLKADVCAT_INFO advcat_clk;
	map<int, vector<int> > creative_cat_index; // map内容：行业编号 属于该行业的所有创意

#ifdef DEBUG
	for (int n = 0; n < index.size(); ++n){
		cout << "index :" << index[n] << " mapid :" << v_creative[n].mapid << endl;
	}
#endif

	//提取创意所属广告行业
	for (int n = 0; n < index.size(); ++n)
	{
		const CREATIVEOBJECT &crev = v_creative[index[n]];
		creative_cat_index[crev.advcat].push_back(n);
	}

	//所有创意属于同一个行业，随机选
	if (creative_cat_index.size() == 1)
	{
		cre_index = get_random_index(index.size());
		set_last_filter_err(m_err_group, v_creative, cre_index, index, advcat_cre_index);
		return cre_index;
	}

	struct timespec ts1;
	getTime(&ts1);
	int ret = getUerCLKADVCATinfo(redis_user_advcat, fctx->logid, crequest.device.deviceid, advcat_clk);
	record_cost(logid, fctx->is_print_time, id, "ur_clk_advcat", ts1);
	va_cout("ret :0x%x\n", ret);
	if (ret != E_SUCCESS)
	{
		cre_index = get_random_index(index.size());
		set_last_filter_err(m_err_group, v_creative, cre_index, index, advcat_cre_index);
		return cre_index;
	}

	map<double, vector<string>, greater<double> > m_v_creative;//排序不同广告行业夹角余弦值
	map<double, vector<string>, greater<double> >::iterator m_v_creative_iter;
	map<int, vector<int> >::iterator creative_cat_iter;
	set<int>::iterator it_set;
	pthread_mutex_lock(&fctx->advcat_mutex);
	map<string, double> &advcat = fctx->advcat;

#ifdef DEBUG
	for (it_set = advcat_clk.clkadvcat.begin(); it_set != advcat_clk.clkadvcat.end(); ++it_set)
	{
		cout << "clkadvcat : " << *it_set;
	}
	creative_cat_iter = creative_cat_index.begin();
	while (creative_cat_iter != creative_cat_index.end())
	{
		vector<int> &v_index = creative_cat_iter->second;
		cout << "cre_adv_cat :" << creative_cat_iter->first << "   :  ";
		for (int n = 0; n < v_index.size(); ++n)
		{
			cout << " ," << v_index[n] << endl;
		}
		++creative_cat_iter;
	}
#endif

	for (creative_cat_iter = creative_cat_index.begin(); creative_cat_iter != creative_cat_index.end(); ++creative_cat_iter)
	{
		//如果用户之前点击过该行业，忽略
		if (advcat_clk.clkadvcat.count(creative_cat_iter->first) != 0){
			continue;
		}

		for (it_set = advcat_clk.clkadvcat.begin(); it_set != advcat_clk.clkadvcat.end(); ++it_set)
		{
			char ch[128] = "";
			sprintf(ch, "%dx%d", creative_cat_iter->first, *it_set);
			//1.不相关的广告行业（value为-1）不存入redis中,暂时不加检查
			if (advcat.find(ch) != advcat.end())
			{
				double value = advcat[ch];
				m_v_creative[value].push_back(ch);
			}
		}
	}

	pthread_mutex_unlock(&fctx->advcat_mutex);
	writeLog(logid, LOGINFO, "m_v_creative size :%d", m_v_creative.size());
	if (m_v_creative.size() == 0)
	{
		cre_index = get_random_index(index.size());
		set_last_filter_err(m_err_group, v_creative, cre_index, index, advcat_cre_index);
		return cre_index;
	}

	//获取夹角余弦最小的广告行业
	m_v_creative_iter = m_v_creative.begin();
	cflog(fctx->logid, LOGDEBUG, "id:%s max ctr:%lf", id, m_v_creative_iter->first);
	vector<string> &v_relation_advcat = m_v_creative_iter->second;
	if (v_relation_advcat.size() == 0)
	{
		cre_index = get_random_index(index.size());
		set_last_filter_err(m_err_group, v_creative, cre_index, index, advcat_cre_index);
		return cre_index;
	}

	int cre_cat, user_cat;
	vector<int> mapids;
	set<int> cat_uniq;
	for (int n = 0; n < v_relation_advcat.size(); ++n)
	{
		va_cout("desc :%s", v_relation_advcat[n].c_str());
		cre_cat = user_cat = 0;
		sscanf(v_relation_advcat[n].c_str(), "%dx%d", &cre_cat, &user_cat);
		cat_uniq.insert(cre_cat);
	}

	set<int>::const_iterator it_cat_uniq;
	for (it_cat_uniq = cat_uniq.begin(); it_cat_uniq != cat_uniq.end(); it_cat_uniq++)
	{
		va_cout("uniq desc 0x%x", *it_cat_uniq);
		vector<int> &v_cre_cat_mapids = creative_cat_index[*it_cat_uniq];
		mapids.insert(mapids.end(), v_cre_cat_mapids.begin(), v_cre_cat_mapids.end());
	}

#ifdef DEBUG
	for (int n = 0; n < mapids.size(); ++n){
		cout << "n :" << n << " mapids : " << mapids[n] << endl;
	}
#endif

	cre_index = mapids[get_random_index(mapids.size())];
	crequest.is_recommend = 1; //标记采用智能推荐
	advcat_cre_index.insert(mapids.begin(), mapids.end());
	set_last_filter_err(m_err_group, v_creative, cre_index, index, advcat_cre_index);
	return cre_index;
}

void set_last_filter_err(INOUT map<string, ERRCODE_GROUP> &m_err_group, IN const vector<CREATIVEOBJECT> &v_creative, 
	IN int sel, IN const vector<int> &index, IN const set<int> &index_advcat)
{
	if (index_advcat.size() > 0) // 有行业关联度
	{
		for (int i = 0; i < index.size(); i++)
		{
			const CREATIVEOBJECT &creative = v_creative[index[i]];
			set<int>::const_iterator it;

			if (sel == i)
			{
				m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, 0));
				continue;
			}

			int err = E_FILTER_ADVCAT;
			it = index_advcat.find(i);
			if (it != index_advcat.end()){
				err = E_FILTER_LAST_RANDOM;
			}
			m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, err));
		}

		return;
	}

	// 无行业关联度判断
	for (uint32 i = 0; i < index.size(); i++)
	{
		const CREATIVEOBJECT &creative = v_creative[index[i]];
		if (sel == i)
		{
			m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, 0));
			continue;
		}

		m_err_group[creative.groupid].m_err_creative.insert(make_pair(creative.mapid, E_FILTER_LAST_RANDOM));
	}
}

static bool filter_group_by_target(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN const COM_REQUEST &crequest, 
								   IN const vector<pair<string, GROUPINFO> > &v_p_groupinfo, 
								   OUT vector<pair<string, GROUPINFO> > &v_p_groupinfo_target, 
								   INOUT map<string, ERRCODE_GROUP> &m_err_group)
{
	uint64_t logid = fctx->logid;

	char *id = (char *)crequest.id.c_str();

	if ((fctx == NULL) || (redis_cashe == NULL))
	{
		cflog(logid, LOGWARNING, "%s, filter_group_by_target(fctx:0x%x, redis_cashe:0x%x) param failed!", id, fctx, redis_cashe);
		return false;
	}

	for (int i = 0; i < v_p_groupinfo.size(); i++)
	{
		int target_errcode = E_SUCCESS;

		const string &groupid = v_p_groupinfo[i].first;

		const char *szgroupid = (char *)groupid.c_str();

		map<string, GROUP_TARGET>::iterator it = redis_cashe->group_target.find(groupid);
		if (it == redis_cashe->group_target.end())
		{
			cflog(logid, LOGINFO, "%s, groupid:%s, have no target.", id, szgroupid);
			continue;
		}

		ERRCODE_GROUP err_group = { 0 };

		//1.Filter group with scene target by group target.
		target_errcode = filter_scene_target(fctx, redis_cashe, crequest, szgroupid, it->second.scene_target);
		if (target_errcode != E_SUCCESS)
		{
			cflog(logid, LOGWARNING, "%s, groupid:%s, filter_scene_target errcode:0x%x", id, szgroupid, target_errcode);

			err_group.errcode = target_errcode;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}

		//2.Filter group with app target by group target.
		target_errcode = filter_app_target(fctx, redis_cashe, crequest, szgroupid, it->second.app_target);
		if (target_errcode != E_SUCCESS)
		{
			cflog(logid, LOGWARNING, "%s, groupid:%s, filter_app_target errcode:0x%x", id, szgroupid, target_errcode);
			err_group.errcode = target_errcode;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}

		//3.Filter group with device target by group target.
		target_errcode = filter_device_target(fctx, redis_cashe, crequest, szgroupid, it->second.device_target);
		if (target_errcode != E_SUCCESS)
		{
			cflog(logid, LOGWARNING, "%s, groupid:%s, filter_device_target errcode:0x%x", id, szgroupid, target_errcode);

			err_group.errcode = target_errcode;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}

		v_p_groupinfo_target.push_back(v_p_groupinfo[i]);
	}

	cflog(logid, LOGINFO, "%s, leave fn:%s, ln:%d, v_p_groupinfo_target.size:%d", id, __FUNCTION__, __LINE__, v_p_groupinfo_target.size());

	return v_p_groupinfo_target.size() > 0;
}


static bool filter_request_by_adx_target(IN FILTERCONTEXT *fctx, IN REDIS_INFO *redis_cashe, IN const COM_REQUEST &crequest)
{
	uint64_t logid = fctx->logid;

	char *id = (char *)crequest.id.c_str();

	if ((fctx == NULL) || (redis_cashe == NULL))
	{
		cflog(logid, LOGWARNING, "%s, filter_request_by_adx_target(fctx:0x%x, redis_cashe:0x%x) param failed!", id, fctx, redis_cashe);
		return false;
	}

	ADX_TARGET &adx_target = redis_cashe->adx_target;

	//1.Filter group with app target by group target.
	int target_errcode = filter_app_target(fctx, redis_cashe, crequest, "creq", adx_target.app_target);
	if (target_errcode != E_SUCCESS)
	{
		cflog(logid, LOGWARNING, "%s, groupid:%s, check_app_target errcode:0x%x", id, "creq", target_errcode);
		return false;
	}

	//2.Filter group with device target by group target.
	target_errcode = filter_device_target(fctx, redis_cashe, crequest, "creq", adx_target.device_target);
	if (target_errcode != E_SUCCESS)
	{
		cflog(logid, LOGWARNING, "%s, groupid:%s, check_device_target errcode:0x%x", id, "creq", target_errcode);
		return false;
	}

	return true;
}

static bool filter_group_by_bcat_badv(IN uint64_t logid, IN const vector<pair<string, GROUPINFO> > &v_gpinfo, 
									  IN const COM_REQUEST &request, IN CALLBACK_CHECK_GROUP_EXT callback_check_group_ext, 
									  OUT vector<pair<string, GROUPINFO> > &out_v_gpinfo, INOUT map<string, ERRCODE_GROUP> &m_err_group)
{
	char *id = (char *)request.id.c_str();
	const set<uint32_t> &bcat = request.bcat;
	const set<string> &badv = request.badv;

	for (int i = 0; i < v_gpinfo.size(); i++)
	{
		ERRCODE_GROUP err_group = {0};
		const string &groupid = v_gpinfo[i].first;
		const GROUPINFO &groupinfo = v_gpinfo[i].second;

		if(request.at != groupinfo.at)
		{
			err_group.errcode = E_FILTER_GROUP_BID;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}

		if (request.imp[0].dealprice.size() > 0 && (groupinfo.dealid == "" || !request.imp[0].dealprice.count(groupinfo.dealid)))
		{
			err_group.errcode = E_FILTER_GROUP_PMP_DEALID;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}

		if(request.is_secure == true && groupinfo.is_secure == false)
		{
			err_group.errcode = E_FILTER_GROUP_SECURE;
			m_err_group.insert(make_pair(groupid, err_group));
			continue;
		}

		if (callback_check_group_ext != NULL)
		{
			if (!callback_check_group_ext(request, groupinfo.ext))
			{
				cflog(logid, LOGWARNING, "%s,group:%s ext error", id, groupid.c_str());

				err_group.errcode = E_FILTER_GROUP_EXT;
				m_err_group.insert(make_pair(groupid, err_group));
				continue;
			}
		}

		set<string>::iterator badv_it = badv.find(groupinfo.adomain);
		if(badv_it != badv.end())
		{
			string string_s_badv = "";

			set<string>::iterator it = badv.begin();

			for (; it != badv.end(); it++)
				string_s_badv += (" / " + *it);

			cflog(logid, LOGWARNING, "%s, groupinfo[%s].adomain:%s in request's badv:%s", 
				id, groupid.c_str(), groupinfo.adomain.c_str(), string_s_badv.c_str());

			err_group.errcode = E_FILTER_BADV;
			m_err_group.insert(make_pair(groupid, err_group));

			continue;
		}

		if (!filter_request_bcat(logid, bcat, groupinfo.cat))
		{
			cflog(logid, LOGWARNING, "groupinfo[%s].cat in request's bcat", groupid.c_str());

			err_group.errcode = E_FILTER_BCAT;
			m_err_group.insert(make_pair(groupid, err_group));

			continue;
		}

		out_v_gpinfo.push_back(v_gpinfo[i]);
	}

	return out_v_gpinfo.size() > 0;
}

static int filter_group(IN uint64_t ctx, IN uint8_t index, IN REDIS_INFO *redis_cashe, IN const set<string> &s_finished_groupids,
						IN CALLBACK_CHECK_GROUP_EXT callback_check_group_ext,
						IN CALLBACK_CHECK_PRICE callback_check_price, 
						IN CALLBACK_CHECK_EXT callback_check_ext,
						IN bool bwhitelist, INOUT COM_REQUEST &crequest, INOUT USER_FREQUENCY_COUNT &ur, 
						OUT COM_BIDOBJECT &dst_cbid, INOUT ERRCODE_GROUP_FAMILY &err_group_family)
{
	int errcode = E_SUCCESS;
	FILTERCONTEXT *fctx = (FILTERCONTEXT *)ctx;
	uint64_t logid = fctx->logid;
	char *id = (char *)crequest.id.c_str();
	const char *sequence = (bwhitelist ? "first" : "second");    
	bool b_f_gp_cat_adv = false, b_f_gb_wbl = false, b_gp_target = false, b_creative = false;
	vector<pair<string, GROUPINFO> > available_groupinfo;	
	vector<pair<string, GROUPINFO> > v_p_groupinfo_temp, v_p_groupinfo;
	vector<pair<string, GROUPINFO> > &temp = (bwhitelist ? redis_cashe->wgroupinfo : redis_cashe->groupinfo);
	timespec ts1;
	map<string, bool> group_has_blist;

	if (temp.size() == 0)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, [%s], groupinfo.size():%d", 
			id, __FUNCTION__, __LINE__, sequence, (bwhitelist ? redis_cashe->wgroupinfo.size() : redis_cashe->groupinfo.size()));

		errcode = E_FILTER_HAVE_NO_GROUP;
		goto exit;
	}

	for (int i = 0; i < temp.size(); i++)
	{
		set<string>::const_iterator it = s_finished_groupids.find(temp[i].first);
		if (it == s_finished_groupids.end())
		{
			available_groupinfo.push_back(temp[i]);
		}
		else
		{
			ERRCODE_GROUP err_group = { 0 };
			err_group.errcode = E_FILTER_FRC_GROUP;
			err_group_family.m_err_group.insert(make_pair(temp[i].first, err_group));
		}
	}

	if (available_groupinfo.size() == 0)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, [%s], available_groupinfo.size():%d", 
			id, __FUNCTION__, __LINE__, sequence, available_groupinfo.size());

		errcode = E_FILTER_HAVE_NO_AVAILABLE_GROUP;
		goto exit;
	}

	getTime(&ts1);
	b_f_gp_cat_adv = filter_group_by_bcat_badv(logid, available_groupinfo, 
											crequest, callback_check_group_ext, 
											v_p_groupinfo_temp, err_group_family.m_err_group);
	record_cost(logid, fctx->is_print_time, id, "f_gp_cat_adv", ts1);

	if (!b_f_gp_cat_adv)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, [%s], filter_bcat_and_badv_grext failed!", 
			id, __FUNCTION__, __LINE__, sequence);

		dump_set_cat_adv(logid, LOGWARNING, id, crequest.bcat, crequest.badv);

		errcode = E_FILTER_BCAT_OR_BADV_GREXT;
		goto exit;
	}

	cflog(logid, LOGINFO, "%s, %s/%d, [%s], after filter_group_by_bcat_badv, v_p_groupinfo_temp.size:%d", 
		id, __FUNCTION__, __LINE__, sequence, v_p_groupinfo_temp.size());	

	getTime(&ts1);

	b_gp_target = filter_group_by_target(fctx, redis_cashe, crequest, 
										v_p_groupinfo_temp, v_p_groupinfo, 
										err_group_family.m_err_group);
	record_cost(logid, fctx->is_print_time, id, "f_gp_target", ts1);

	if (!b_gp_target)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, [%s], f_gp_target failed!", 
			id, __FUNCTION__, __LINE__, sequence);

		dump_vector_pair_groupinfo(logid, LOGWARNING, id, v_p_groupinfo_temp);

		errcode = E_FILTER_TARGET;
		goto exit;
	}

	if (bwhitelist)
	{
		uint8_t deviceidtype = DPID_UNKNOWN;
		string deviceid = "";
		vector<pair<string, GROUPINFO> > v_p_groupinfo_wb_temp;
		getTime(&ts1);
		errcode = filter_group_by_wlist(logid, redis_cashe, 
			fctx->redis_fds[index].redis_wblist_detail, crequest, 
			v_p_groupinfo, 
			v_p_groupinfo_wb_temp, group_has_blist,
			deviceidtype, deviceid, 
			err_group_family.m_err_group, fctx->is_print_time);//pick only lucky one.

		record_cost(logid, fctx->is_print_time, id, "f_gb_wbl", ts1);

		if(errcode == E_REDIS_CONNECT_INVALID)
		{
			goto exit;
		}
		else if(errcode == E_SUCCESS)
		{
			v_p_groupinfo = v_p_groupinfo_wb_temp;
			crequest.device.deviceid = deviceid;
			crequest.device.deviceidtype = deviceidtype;
		}
		else
		{
			cflog(logid, LOGERROR, "%s, %s/%d, [%s], filter_wl_bl failed!", 
				id,  __FUNCTION__, __LINE__, sequence);

			dump_vector_pair_groupinfo(logid, LOGWARNING, id, v_p_groupinfo_temp);

			errcode = E_FILTER_GROUP_WBL;
			goto exit;
		}
	}
	else
	{
		const map<uint8_t, string> &ids = (crequest.device.dpids.size() > 0) ? crequest.device.dpids : crequest.device.dids;

		crequest.device.deviceidtype = ids.begin()->first;
		crequest.device.deviceid = ids.begin()->second;
	}

	if (ur.userid != crequest.device.deviceid)
	{
		clear_ur(ur);
	}

	getTime(&ts1);
	b_creative = filter_creative(fctx, redis_cashe, fctx->redis_fds[index].redis_ur,
		fctx->redis_fds[index].redis_user_advcat, crequest, 
		callback_check_price, callback_check_ext, 
		v_p_groupinfo, group_has_blist,
		bwhitelist, ur,
		dst_cbid, err_group_family.m_err_group);

	record_cost(logid, fctx->is_print_time, id, "f_bid", ts1);
	if (!b_creative)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, [%s], filter creative failed", 
			id, __FUNCTION__, __LINE__, sequence);
		errcode = E_FILTER_HAVE_NO_CREATIVE;
		goto exit;
	}

exit:
	err_group_family.errcode = errcode;
	return errcode;
}

static int get_finished_groupids(IN uint64_t ctx, IN uint8_t index, IN REDIS_INFO *redis_cashe, OUT set<string> &s_groupids)
{
	int period_array[] = {15, 30, 60};
	int errcode = E_SUCCESS;
	const char *dst_array[] = {"error", "dstimp", "dstclk"};
	const char *sum_array[] = { "error", "sumimp", "sumclk" };
	const char *type_array[] = { "error", "imp", "clk" };
	const char *finish_array[] = { "error", "s_imp_t", "s_clk_t" };
	int denominator_array[] = {
		4, //RESTRICTION_PERIOD_15MIN
		2, //RESTRICTION_PERIOD_30MIN
		1
	};//RESTRICTION_PERIOD_HOUR

	FILTERCONTEXT *fctx = (FILTERCONTEXT *)ctx;

	struct timespec ts1;
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	char m_day_key[MIN_LENGTH] = "";
	sprintf(m_day_key, "dsp_group_frequency_%02d_%02d", fctx->adx, tm.tm_mday);

	map<string, string> m_group_frequency;
	map<string, BID_INFO> m_group_bid_info;  // 存储项目竞价信息
	map<string, BID_INFO>::iterator p_bid_info;

	getTime(&ts1);
	errcode = hgetallRedisValue(fctx->redis_fds[index].redis_gr, fctx->logid, string(m_day_key), m_group_frequency);
	if(errcode != E_SUCCESS)
	{
		goto exit;
	}
	record_cost(fctx->logid, fctx->is_print_time, "", "hgetall", ts1);

	if (m_group_frequency.size() > 0)
	{
		for (map<string, string>::iterator it = m_group_frequency.begin(); it != m_group_frequency.end(); it++)
		{
			string groupid = it->first;

			json_t *root = NULL;
			json_parse_document(&root, it->second.c_str());
			if(root != NULL)
			{
				map<string, FREQUENCYCAPPINGRESTRICTION>::iterator it_capping = redis_cashe->fc.find(groupid);
				if (it_capping != redis_cashe->fc.end())
				{
					map<uint8_t, FREQUENCY_RESTRICTION_GROUP> &map_frg = it_capping->second.frg;
					if (!map_frg.count(fctx->adx))   // 平台不存在，不投放
					{
						s_groupids.insert(groupid);
						goto loopend;
					}
					else
					{
						FREQUENCY_RESTRICTION_GROUP &frg = map_frg[fctx->adx];
						if (frg.type != GROUP_FREQUENCY_INFINITY)
						{
							if ( frg.period == RESTRICTION_PERIOD_DAY)
							{
								if (frg.frequency.size() < 1 || frg.frequency[0] == 0)
								{
									cflog(fctx->logid, LOGERROR, "%s, %s/%d, frg.type=%d,frg.period=%d,frg.frequency.size=%d is less or set data is zero!",
										"", __FUNCTION__, __LINE__, frg.type, frg.period, frg.frequency.size());

									s_groupids.insert(groupid);
									goto loopend;
								}
							}
							else
							{
								if (frg.frequency.size() <= tm.tm_hour || frg.frequency[tm.tm_hour] == 0)
								{
									cflog(fctx->logid, LOGERROR, "%s, %s/%d, frg.type=%d,frg.period=%d,frg.frequency.size=%d is less or set data is zero!",
										"", __FUNCTION__, __LINE__, frg.type, frg.period, frg.frequency.size());

									s_groupids.insert(groupid);
									goto loopend;
								}
							}

							uint64_t sum_impclick_count = 0, dst_impclick_count = frg.frequency[0];
							char sum_impclick[MIN_LENGTH] = "";

							sprintf(sum_impclick, "%s", sum_array[frg.type]);
							if (frg.period != RESTRICTION_PERIOD_DAY)   // 其他方式
							{
								dst_impclick_count = frg.frequency[tm.tm_hour];
								sprintf(sum_impclick, "%s-%02d", sum_array[frg.type], tm.tm_hour);
							}

							json_t *label_sum_impclick = NULL;
							if ((label_sum_impclick = json_find_first_label(root, sum_impclick)) != NULL)//跨小时临界点，新小时数据可能读不到
							{
								sum_impclick_count = atoi(label_sum_impclick->child->text);
							}

							if (frg.period != RESTRICTION_PERIOD_DAY)
							{
								json_t *label_dst_impclick = NULL;
								char dst_impclick[MIN_LENGTH] = "";
								sprintf(dst_impclick, "%s-%02d", dst_array[frg.type], tm.tm_hour);

								if ((label_dst_impclick = json_find_first_label(root, dst_impclick)) != NULL)
								{
									dst_impclick_count = atoi(label_dst_impclick->child->text);
								}
								BID_INFO bid_info;
								bid_info.finish_time = INIT_FINISH_TIME;
								bid_info.curr_ratio = INIT_RATIO;
								sprintf(dst_impclick, "%s_%02d", finish_array[frg.type], tm.tm_hour);
								if ((label_dst_impclick = json_find_first_label(root, dst_impclick)) != NULL)
								{
									bid_info.finish_time = atoi(label_dst_impclick->child->text);
								}
								m_group_bid_info.insert(make_pair(groupid,bid_info));
							}

							if (sum_impclick_count >= dst_impclick_count)//前几个时间片已达到当前时段总频次
							{
								cflog(fctx->logid, LOGERROR, "%s, %s/%d, sum_impclick_count:%d, dst_impclick_count:%d",
									"", __FUNCTION__, __LINE__, sum_impclick_count, dst_impclick_count);

								s_groupids.insert(groupid);
								goto loopend;
							}

							if (frg.period == RESTRICTION_PERIOD_DAY)
								goto loopend;

							json_t *label_cur_interval = NULL;
							char cur_interval[MIN_LENGTH] = "";
							sprintf(cur_interval, "%02d%02d-%02d",
								tm.tm_hour,
								period_array[frg.period] * (tm.tm_min / period_array[frg.period]),
								period_array[frg.period]);
							if ((label_cur_interval = json_find_first_label(root, cur_interval)) != NULL)
							{
								uint32_t max_frequency = dst_impclick_count / denominator_array[frg.period];

								json_t *label_impclick = NULL;
								if((label_impclick = json_find_first_label(label_cur_interval->child, type_array[frg.type])) != NULL)
								{
									if (atoi(label_impclick->child->text) > max_frequency)//已达到当前时段频次
									{
										cflog(fctx->logid, LOGERROR, "%s, %s/%d, cur_interval(%s)-impclick:%s, max_frequency:%d",
											"", __FUNCTION__, __LINE__, cur_interval, label_impclick->child->text, max_frequency);

										s_groupids.insert(groupid);
										goto loopend;
									}
								}
							}
						}
						else
						{
							if (frg.period != RESTRICTION_PERIOD_DAY)
							{
								if (frg.frequency.size() <= tm.tm_hour)
								{
									cflog(fctx->logid, LOGERROR, "%s, %s/%d, frg.type=%d,frg.period=%d,frg.frequency.size=%d is less!",
										"", __FUNCTION__, __LINE__, frg.type, frg.period, frg.frequency.size());

									s_groupids.insert(groupid);
									goto loopend;

								}
								if (frg.frequency[tm.tm_hour] == 0) // 1，无限投，0，不投
								{
									cflog(fctx->logid, LOGERROR, "%s, %s/%d, frg.frequency[%d]=%d,frg.type=%d",
										"", __FUNCTION__, __LINE__, tm.tm_hour, frg.frequency[tm.tm_hour],frg.type);

									s_groupids.insert(groupid);
									goto loopend;
								}
							}
						}
					}
				}
			}
loopend:
			if (root != NULL)
				json_free_value(&root);
		} // end 遍历 map<string, string> m_group_frequency
	}

	pthread_mutex_lock(&fctx->bid_info_mutex);
	if (tm.tm_hour == 0)
	{
		if (fctx->zero_flag == 0)
		{
			fctx->zero_flag = 1;
			fctx->group_bid_info.clear();
			pthread_mutex_unlock(&fctx->bid_info_mutex);
			cflog(fctx->logid, LOGDEBUG, "clear success!");
			goto exit;
		}
	}
	else
	{
		fctx->zero_flag = 0;
	}
	{
		map<string, map<int, BID_INFO> > &m_info = fctx->group_bid_info;
		for (p_bid_info = m_group_bid_info.begin(); p_bid_info != m_group_bid_info.end(); ++p_bid_info)
		{
			string groupid = p_bid_info->first;
			if (!redis_cashe->grsmartbidinfo.count(groupid) || redis_cashe->grsmartbidinfo[groupid].flag == 0)  // 不参与智能出价
			{
				map<int, BID_INFO> &curr = m_info[groupid];
				curr.clear();
				continue;
			}
			/* if (!m_info.count(groupid)) // 没有该项目
			{
			map<int, BID_INFO> curr;
			curr.insert(make_pair(tm.tm_hour, p_bid_info->second));
			m_info.insert(make_pair(groupid, curr));
			}*/
			//    else
			{
				map<int, BID_INFO> &curr = m_info[groupid];
				if (!curr.count(tm.tm_hour))
				{
					curr[tm.tm_hour] = p_bid_info->second;
				}
				else
				{
					BID_INFO &bid_tmp = curr[tm.tm_hour];
					//	 if (bid_tmp.finish_time == INIT_FINISH_TIME)    // 覆盖结束时间，price的值不修改
					{
						bid_tmp.finish_time = p_bid_info->second.finish_time;
					}
				}
			}
		}
		pthread_mutex_unlock(&fctx->bid_info_mutex);
	}
exit:
	return errcode;
}

static int read_smart_ratio(FILTERCONTEXT *ctx, REDIS_INFO *redis_cashe)
{	
	int errcode = E_SUCCESS;
	char key[MID_LENGTH];
	string smart_value;
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);
	int last_hour = tm.tm_hour - 1;

	if ((ctx == NULL) || (redis_cashe == NULL))
	{	
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if (ctx->redis_smart == NULL)
	{
		errcode = E_REDIS_MASTER_FILTER_SMART;
		cflog(ctx->logid, LOGERROR, "%s/%d,redis_smart:0x%x",__FUNCTION__, __LINE__, ctx->redis_smart);
		goto exit;
	}

	for (int i = 0; i < redis_cashe->groupinfo.size(); ++i)
	{
		pair<string, GROUPINFO> &groupinfo = redis_cashe->groupinfo[i];
		string groupid = groupinfo.first;

		if (!redis_cashe->grsmartbidinfo.count(groupid) || redis_cashe->grsmartbidinfo[groupid].flag == 0)
		{
			continue;
		}

		sprintf(key, "dsp_groupid_smart_%02d_%s_%02d", ctx->adx, groupid.c_str(), tm.tm_mday);

		errcode = getRedisValue(ctx->redis_smart, ctx->logid, key, smart_value);
		if(errcode == E_REDIS_CONNECT_INVALID)
			goto exit;

		if (smart_value != "")
		{
			json_t *root = NULL, *label = NULL;
			json_parse_document(&root, smart_value.c_str());
			if (root == NULL)
			{
				cflog(ctx->logid, LOGERROR, "%s/%d,key:%s,parse error",__FUNCTION__, __LINE__, key);
				continue;
			}

			if ((label = json_find_first_label(root, groupid.c_str())) != NULL)
			{
				json_t *ratio_value = label->child;
				//	sprintf(key, "%02d", tm.tm_hour);

				pthread_mutex_lock(&ctx->bid_info_mutex);
				map<string, map<int, BID_INFO> > &m_info = ctx->group_bid_info;
				map<int, BID_INFO> &curr = m_info[groupid];

				for (int j = tm.tm_hour; j >= 0; --j)
				{
					BID_INFO bid;
					sprintf(key, "%02d", j);
					if ((label = json_find_first_label(ratio_value, key)) != NULL)
					{
						if (j == tm.tm_hour)
						{
							bid.finish_time = INIT_FINISH_TIME;			// 参与竞价时重新获取该值
						}
						else
						{
							bid.finish_time = redis_cashe->grsmartbidinfo[groupid].interval + 1;  // 这个小时按照上个小时的出价
						}
						bid.curr_ratio = atof(label->child->text);
						curr.insert(make_pair(j, bid));
					}
				}
unlock:
				pthread_mutex_unlock(&ctx->bid_info_mutex);
			}

			if (root != NULL)
				json_free_value(&root);
		}
	}

exit:
	return errcode;
}

static int set_instl(IN uint64_t ctx, IN const map<string, uint8_t> &img_imptype, INOUT COM_REQUEST &crequest)
{
	int errcode = E_REQUEST_IMP_INSTL_SIZE_NOT_FIND;
	FILTERCONTEXT *fctx = (FILTERCONTEXT *)ctx;

	if (crequest.imp.size() > 0)
	{
		COM_IMPRESSIONOBJECT &cimp = crequest.imp[0];
		int w = 0, h = 0;
		switch (cimp.type)
		{
		case IMP_BANNER:
			{
				w = cimp.banner.w;
				h = cimp.banner.h;   
				break;
			}	
		default:
			{
				errcode = E_SUCCESS;
				goto exit;
			}
		}
		char imgsize[MIN_LENGTH];
		sprintf(imgsize,"%dx%d",w,h);
		map<string, uint8_t>::const_iterator pfind = img_imptype.find(imgsize);
		if ( pfind != img_imptype.end())
		{
			if ( cimp.ext.instl == INSTL_NO)
				cimp.ext.instl = pfind->second;

			errcode = E_SUCCESS;
		}
		else
		{
			cflog(fctx->logid, LOGERROR, "%s, %s/%d, instl size=%s not find!",crequest.id.c_str(), __FUNCTION__, __LINE__, imgsize);	
		}
	}

exit:
	return errcode;
}

FULL_ERRCODE bid_filter_response(IN uint64_t ctx, IN uint8_t index, INOUT COM_REQUEST &crequest, 
								 IN CALLBACK_PROCESS_CREQUEST callback_process_crequest, 
								 IN CALLBACK_CHECK_GROUP_EXT callback_check_group_ext,
								 IN CALLBACK_CHECK_PRICE callback_check_price, 
								 IN CALLBACK_CHECK_EXT callback_check_ext,
								 OUT COM_RESPONSE *pcresponse,
								 OUT ADXTEMPLATE *padxtemplate)
{
	FULL_ERRCODE ferrcode;
	int errcode = E_SUCCESS;
	int pipe_err_temp = E_SUCCESS;
	uint64_t redis_info_addr = 0, logid = 0;
	sem_t sem_id;
	FILTERCONTEXT *fctx = (FILTERCONTEXT *)ctx;
	REDIS_INFO *redis_cashe = NULL;
	COM_BIDOBJECT dst_cbid;
	char *id = (char *)crequest.id.c_str();
	COM_SEATBIDOBJECT seatbid;
	USER_FREQUENCY_COUNT ur;	
	set<string> s_finished_groupids;
	timespec ts1;
	struct tm tm = {0};
	time_t timep;
	time(&timep);

	set<string>::iterator s_string_it;
	int i = 0;
	int ferr_level = FERR_LEVEL_TOP;

	fctx->is_print_time = 0;
	if (fctx == 0)
	{
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if ((logid = fctx->logid) == 0)
	{
		errcode = E_BID_PROGRAM;
		goto exit;
	}

	cflog(logid, LOGINFO, "%s, %s/%d, --Enter", 
		id, __FUNCTION__, __LINE__);

	if (pcresponse == NULL)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, pcresponse == NULL", 
			id, __FUNCTION__, __LINE__);

		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if (padxtemplate == NULL)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, padxtemplate == NULL", 
			id, __FUNCTION__, __LINE__);

		errcode = E_BID_PROGRAM;
		goto exit;
	}

	localtime_r(&timep, &tm);

	if (tm.tm_hour == 6)
	{
		pthread_mutex_lock(&fctx->advcat_mutex);
		if (fctx->timer_recommend == 0)
		{
			fctx->timer_recommend = 1;
			fctx->advcat.clear();
			errcode = getADVCAT(fctx->redis_relation_advcat, fctx->logid, fctx->advcat);
			if(errcode == E_REDIS_CONNECT_INVALID)
			{
				pthread_mutex_unlock(&fctx->advcat_mutex);
				errcode = E_BID_PROGRAM;
				goto exit;
			}
		}
		pthread_mutex_unlock(&fctx->advcat_mutex);
	}
	else
	{
		pthread_mutex_lock(&fctx->advcat_mutex);
		fctx->timer_recommend = 0;
		pthread_mutex_unlock(&fctx->advcat_mutex);
	}

	if (crequest.is_secure != 0)
	{
		cflog(logid, LOGDEBUG, "%s is secure,%d", id, crequest.is_secure);
	}
	getTime(&ts1);
	errcode = check_crequest_validity(crequest);
	record_cost(logid, fctx->is_print_time, id, "ck_req", ts1);
	if (errcode != E_SUCCESS)
	{
		cflog(logid, LOGERROR, "%s, %s/%d, ck_req err:0x%x", 
			id, __FUNCTION__, __LINE__,
			errcode);

		goto exit;
	}

	get_semaphore(fctx->cache, &redis_info_addr, &sem_id);

	if(semaphore_release(sem_id))//+1
	{
		redis_cashe = (REDIS_INFO* )redis_info_addr;
		if(redis_cashe->errorcode == E_REDIS_CONNECT_INVALID)
		{
			cflog(logid, LOGDEBUG, "redis_cashe redis context invalid!");
			errcode = E_BID_PROGRAM;
			goto release_resource;
		}
		//parseStruct(redis_info_addr);
		fctx->is_print_time = redis_cashe->logctrlinfo.is_print_time;
		ferr_level = redis_cashe->logctrlinfo.ferr_level;

		if (redis_cashe->adxtemplate.adx == ADX_UNKNOWN)
		{
			cflog(logid, LOGERROR, "%s, %s/%d, cashe->temp err",
				id, __FUNCTION__, __LINE__);
			//cout << " errorcode : "<<hex <<redis_cashe->errorcode <<endl;
			errcode = E_CACHE;
			goto release_resource;
		}

		// set instl
		errcode = set_instl(ctx, redis_cashe->img_imptype, crequest);
		if (errcode != E_SUCCESS)
			goto release_resource;

		// 14 adx 平台定向错误码需重新定义	11-13
		getTime(&ts1);
		bool b_adx_target = filter_request_by_adx_target(fctx, redis_cashe, crequest);
		record_cost(logid, fctx->is_print_time, id, "f_req_adxtarget", ts1);
		if (!b_adx_target)
		{
			cflog(logid, LOGERROR, "%s, %s/%d, f_req_adxtarget failed!", 
				id, __FUNCTION__, __LINE__);

			errcode = E_REQUEST_TARGET_ADX;
			goto release_resource;
		}

		if (callback_process_crequest != NULL)
		{
			getTime(&ts1);
			bool b_req_cb = callback_process_crequest(crequest, redis_cashe->adxtemplate);
			record_cost(logid, fctx->is_print_time, id, "cb_pc", ts1);

			if (!b_req_cb)
			{
				cflog(logid, LOGERROR, "%s, %s/%d, cb_pc(addr:0x%x) failed!", 
					id, __FUNCTION__, __LINE__, callback_process_crequest);

				errcode = E_REQUEST_PROCESS_CB;
				goto release_resource;
			}
		}

		errcode = get_finished_groupids(ctx, index, redis_cashe, s_finished_groupids);
		if(errcode == E_REDIS_CONNECT_INVALID)
		{
			cflog(logid, LOGDEBUG, "group frequency redis context invalid!");
			errcode = E_BID_PROGRAM;
			goto release_resource;
		}

		if (s_finished_groupids.size() > 0)
		{
			string finished_groupids = "";

			for (s_string_it = s_finished_groupids.begin(); s_string_it != s_finished_groupids.end(); s_string_it++)
			{
				finished_groupids += (string(" ") + s_string_it->c_str());
			}

			cflog(logid, LOGERROR, "%s, %s/%d, finished_groupids[%d]:%s", 
				id, __FUNCTION__, __LINE__, 
				s_finished_groupids.size(), finished_groupids.c_str());
		}

		if(ZERO(crequest.device.geo.lon) ||ZERO(crequest.device.geo.lat) ||ZERO(crequest.device.geo.lat - 1.0) || ZERO(crequest.device.geo.lon - 1.0))
		{
			cflog(logid, LOGINFO, "Invalid geo info %lf %lf", crequest.device.geo.lon, crequest.device.geo.lat);
		}
		else
		{
			geohash_encode(crequest.device.geo.lat, crequest.device.geo.lon, crequest.device.locid, DEFAULTLENGTH + 1);
		}

		errcode = filter_group(ctx, index, redis_cashe, s_finished_groupids, 
								callback_check_group_ext, callback_check_price, callback_check_ext, 
								true, crequest, ur, 
								dst_cbid, ferrcode.err_group_family[0]);

		if(errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_BID_PROGRAM;
			goto release_resource;
		}
		else if (errcode != E_SUCCESS)
		{
			errcode = filter_group(ctx, index, redis_cashe, s_finished_groupids, 
									callback_check_group_ext, callback_check_price, callback_check_ext, 
									false, crequest, ur, 
									dst_cbid, ferrcode.err_group_family[1]);

			if (errcode != E_SUCCESS)
			{
				errcode = E_FILTER;
				goto release_resource;
			}
		}

		getTime(&ts1);
		int errcode_adxtemp =  check_adxtemplate_validity(redis_cashe->adxtemplate, crequest, dst_cbid);//保护性检查，防止redis中adx模板错误
		record_cost(logid, fctx->is_print_time, id, "ck_temp", ts1);
		if (errcode_adxtemp != E_SUCCESS)
		{
			cflog(logid, LOGERROR, "%s, %s/%d, ck_temp, err:0x%x", 
				id, __FUNCTION__, __LINE__, errcode_adxtemp);

			errcode = errcode_adxtemp;
			goto release_resource;
		}	

		*padxtemplate = redis_cashe->adxtemplate;//if success, then copy adxtemplate.

		if(crequest.is_secure == 1)
		{
			padxtemplate->aurl = fctx->active_https_server + padxtemplate->aurl;
			padxtemplate->nurl = fctx->settle_https_server + padxtemplate->nurl;
			padxtemplate->iurl[0] = fctx->impression_https_server + padxtemplate->iurl[0];
			padxtemplate->cturl[0] = fctx->click_https_server + padxtemplate->cturl[0];
		}
		else
		{
			padxtemplate->aurl = fctx->active_server + padxtemplate->aurl;
			padxtemplate->nurl = fctx->settle_server + padxtemplate->nurl;
			padxtemplate->iurl[0] = fctx->impression_server + padxtemplate->iurl[0];
			padxtemplate->cturl[0] = fctx->click_server + padxtemplate->cturl[0];
		}
		if (dst_cbid.groupid_type == "dsp")
		{	
			////20170509: 暂定不对激活效果地址的修改
			////padxtemplate->aurl = padxtemplate->aurl;
			padxtemplate->nurl = padxtemplate->nurl;
			padxtemplate->iurl[0] = padxtemplate->iurl[0];
			padxtemplate->cturl[0] = padxtemplate->cturl[0];
		}
		else if (dst_cbid.groupid_type == "pap")
		{
			////20170509: 暂定不对激活效果地址的修改
			////padxtemplate->aurl = padxtemplate->aurl";
			padxtemplate->nurl = padxtemplate->nurl;
			padxtemplate->iurl[0] = padxtemplate->iurl[0];
			padxtemplate->cturl[0] = padxtemplate->cturl[0];
		}
	}
	else
	{
		cflog(logid, LOGERROR, "%s, %s/%d, semaphore_release failed!", 
			id, __FUNCTION__, __LINE__);

		errcode = E_BID_PROGRAM;
		goto exit;
	}

release_resource:

	if(semaphore_wait(sem_id))//-1
	{
	}
	else
	{
		cflog(logid, LOGERROR, "%s, %s/%d, semaphore_wait failed!!", 
			id, __FUNCTION__, __LINE__);

		errcode = E_BID_PROGRAM;
		goto exit;
	}

	if (errcode == E_SUCCESS)
	{
		string key_flag = string("dsp_id_flag_") + crequest.id + "_" + dst_cbid.impid;

		pcresponse->id = crequest.id;//Copy request id to response id

		seatbid.bid.push_back(dst_cbid);

		pcresponse->seatbid.push_back(seatbid);

		pcresponse->bidid = crequest.id;

		pcresponse->cur = "CNY";

		getTime(&ts1);

		errcode = setexRedisValue(fctx->redis_fds[index].redis_id, fctx->logid, key_flag,
			crequest.imp[0].requestidlife, intToString(DSP_FLAG_BID));
		record_cost(logid, fctx->is_print_time, id, "set_idf", ts1);

		if (errcode == E_REDIS_CONNECT_INVALID)
			errcode = E_BID_PROGRAM;
	}

exit:
	cflog(logid, LOGINFO, "%s, --leave %s/%d, errcode:0x%x", 
		id, __FUNCTION__, __LINE__, errcode);

	pthread_mutex_lock(&fctx->pipe_err_mutex);
	pipe_err_temp = fctx->pipe_err;
	pthread_mutex_unlock(&fctx->pipe_err_mutex);

	if (pipe_err_temp == E_REDIS_CONNECT_INVALID)
		errcode = E_BID_PROGRAM;

	ferrcode.errcode = errcode;
	ferrcode.level = ferr_level;

	return ferrcode;
}

void get_server_content(uint64_t ctx, uint8_t index, string &data)
{
	static time_t limit_time = 0;
	sem_t sem_id;
	FILTERCONTEXT *fctx = (FILTERCONTEXT *)ctx;
	uint64_t redis_info_addr = 0;
	REDIS_INFO *redis_cashe = NULL;
	set<string> finished;
	time_t t_now = time(0);

	if (t_now - limit_time < 1){
		return;
	}
	limit_time = t_now;

	get_semaphore(fctx->cache, &redis_info_addr, &sem_id);
	if (!semaphore_release(sem_id)){
		return;
	}

	redis_cashe = (REDIS_INFO*)redis_info_addr;
	get_finished_groupids(ctx, index, redis_cashe, finished);

	dump_bid_content(redis_cashe, finished, data);
	data = string("Content-Length: ") + intToString(data.size()) + "\r\n\r\n" + data;

	semaphore_wait(sem_id);
}

static void print_map_bid_ratio_info(IN uint64_t logid, IN uint8_t level, IN const map<string, map<int, BID_INFO> > &ratio_info)
{
	map<string, map<int, BID_INFO> >::const_iterator info = ratio_info.begin();

	for (info; info != ratio_info.end(); ++info)
	{
		const map<int, BID_INFO> &bid = info->second;
		string hour_ratio;
		char temp[MID_LENGTH];
		map<int, BID_INFO>::const_iterator pbid = bid.begin();
		for (pbid; pbid != bid.end(); ++pbid)
		{
			sprintf(temp, "%02d:(%lf_%d),", pbid->first, pbid->second.curr_ratio, pbid->second.finish_time);
			hour_ratio += string(temp);
		}

		cflog(logid, level, "print_map_bid_ratio_info,groupid=%s,hour_ratio=%s", info->first.c_str(), hour_ratio.c_str());
	}
}

int bid_filter_initialize(IN uint64_t logid, IN uint8_t adx, IN uint8_t thread_count, IN bool *run_flag, IN char *priv_conf, 
						  OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx)
{
	int errcode = E_SUCCESS;
	int thrad_value = 0;
	int thrad_value_smart = 0;
	FILTERCONTEXT *ctx = NULL;

	string conf = priv_conf;
	char *global_conf = (char *)conf.c_str();

	uint64_t cache = 0;

	char *master_major_ip = NULL;
	uint16_t master_major_port = 0;

	char *slave_major_ip = NULL;
	uint16_t slave_major_port = 0;

	char *master_filter_id_ip = NULL;
	uint16_t master_filter_id_port = 0;

	char *master_gr_ip = NULL;
	uint16_t master_gr_port = 0;
	char *slave_gr_ip = NULL;
	uint16_t slave_gr_port = 0;

	char *slave_ur_ip = NULL;
	uint16_t slave_ur_port = 0;

	char *slave_wblist_ip = NULL;
	uint16_t slave_wblist_port = 0;

	char *pub_sub_ip = NULL;
	uint16_t pub_sub_port = 0;

	char *settle_server = NULL;
	char *click_server = NULL;
	char *impression_server = NULL;
	char *active_server = NULL;

	char *settle_https_server = NULL;
	char *click_https_server = NULL;
	char *impression_https_server = NULL;
	char *active_https_server = NULL;

	char *slave_advcat_ip = NULL;
	uint16_t slave_advcat_port = 0;
	redisContext *redis_id = NULL;
	redisContext *redis_smart = NULL;
	redisContext *redis_relation_advcat = NULL;

	timespec ts1, ts2;

	uint64_t redis_info_addr = 0;
	REDIS_INFO *redis_cashe = NULL;
	sem_t sem_id;

	ctx = new FILTERCONTEXT;
	if (ctx == NULL)
	{
		errcode = E_MALLOC;
		goto exit;
	}

	ctx->adx = adx;
	ctx->logid = logid;
	ctx->run_flag = run_flag;
	ctx->redis_smart = NULL;
	ctx->redis_relation_advcat = NULL;

	if (logid == 0)
	{
		va_cout("at fn:%s, ln:%d, logid:0x%x", __FUNCTION__, __LINE__, logid);
		errcode = E_INVALID_PARAM_LOGID;
		goto exit;
	}

	if (adx == ADX_UNKNOWN)
	{
		cflog(logid, LOGERROR, "at fn:%s, ln:%d, adx:%d", __FUNCTION__, __LINE__, adx);
		errcode = E_INVALID_PARAM_ADX;
		goto exit;
	}

	if (pctx == NULL)
	{
		cflog(logid, LOGERROR, "at fn:%s, ln:%d, pctx:0x%x", __FUNCTION__, __LINE__, pctx);
		errcode = E_INVALID_PARAM_CTX;
		goto exit;
	}

	master_filter_id_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"master_filter_id_ip");
	master_filter_id_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"master_filter_id_port");

	master_major_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"master_major_ip");
	master_major_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"master_major_port");

	slave_major_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"slave_major_ip");
	slave_major_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"slave_major_port");

	master_gr_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"master_gr_ip");
	master_gr_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"master_gr_port");
	slave_gr_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"slave_gr_ip");
	slave_gr_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"slave_gr_port");

	slave_ur_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"slave_ur_ip");
	slave_ur_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"slave_ur_port");

	slave_wblist_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"slave_wblist_ip");
	slave_wblist_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"slave_wblist_port");

	pub_sub_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"pub_sub_ip");
	pub_sub_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"pub_sub_port");

	slave_advcat_ip = GetPrivateProfileString(global_conf, (char *)"redis", (char *)"slave_advcat_ip");
	slave_advcat_port = GetPrivateProfileInt(global_conf, (char *)"redis", (char *)"slave_advcat_port");

	settle_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"settle_server");
	click_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"click_server");
	impression_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"impression_server");
	active_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"active_server");

	settle_https_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"settle_https_server");
	click_https_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"click_https_server");
	impression_https_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"impression_https_server");
	active_https_server = GetPrivateProfileString(global_conf, (char *)"ad_servers", (char *)"active_https_server");

	if ((master_filter_id_ip == NULL)||(master_filter_id_port == 0)||
		(master_major_ip == NULL)||(master_major_port == 0)||
		(slave_major_ip == NULL)||(slave_major_port == 0)||
		(slave_gr_ip == NULL)||(slave_gr_port == 0)||
		(master_gr_ip == NULL)||(master_gr_port == 0)||
		(slave_ur_ip == NULL)||(slave_ur_port == 0)||
		(pub_sub_ip == NULL)||(pub_sub_port == 0)||
		(slave_wblist_ip == NULL)||(slave_wblist_port == 0)||
		(slave_advcat_ip == NULL)||(slave_advcat_port == 0)||
		(settle_server == NULL)||
		(click_server == NULL)||
		(impression_server == NULL)||
		(active_server == NULL)||
		(settle_https_server == NULL)||
		(click_https_server == NULL)||
		(impression_https_server == NULL)||
		(active_https_server == NULL))
	{
		cflog(logid, LOGERROR, 
			"at fn:%s, ln:%d, \
			master_filter_id_ip:0x%x, master_filter_id_port:%d,\
			slave_major_ip:0x%x, slave_major_port:%d,\
			slave_gr_ip:0x%x, slave_gr_port:%d,\
			master_gr_ip:0x%x, master_gr_port:%d,\
			slave_ur_ip:0x%x, slave_ur_port:%d,\
			pub_sub_ip:0x%x, pub_sub_port:%d,\
			slave_wblist_ip:0x%x, slave_wblist_port:%d,\
			slave_advcat_ip:0x%x, slave_advcat_port:%d,\
			settle_server:0x%x,\
			click_server:0x%x,\
			impression_server:0x%x,\
			active_server:0x%x,\
			settle_https_server:0x%x,\
			click_https_server:0x%x,\
			impression_https_server:0x%x,\
			active_https_server:0x%x",
			__FUNCTION__, __LINE__, 
			master_filter_id_ip, master_filter_id_port,
			slave_major_ip, slave_major_port,
			slave_gr_ip, slave_gr_port,
			master_gr_ip, master_gr_port,
			slave_ur_ip, slave_ur_port,
			pub_sub_ip, pub_sub_port,
			slave_wblist_ip, slave_wblist_port,
			slave_advcat_ip, slave_advcat_port,
			settle_server,
			click_server,
			impression_server,
			active_server,
			settle_https_server,
			click_https_server,
			impression_https_server,
			active_https_server
			);

		errcode = E_INVALID_PROFILE;
		goto exit;
	}

	ctx->settle_server = string(settle_server);
	ctx->click_server = string(click_server);
	ctx->impression_server = string(impression_server);
	ctx->active_server = string(active_server);

	ctx->settle_https_server = string(settle_https_server);
	ctx->click_https_server = string(click_https_server);
	ctx->impression_https_server = string(impression_https_server);
	ctx->active_https_server = string(active_https_server);

	errcode = init_redis_operation(logid, adx, slave_major_ip, slave_major_port, pub_sub_ip, pub_sub_port, run_flag, v_thread_id, &cache);
	if (errcode != E_SUCCESS)
	{
		goto exit;
	}
	ctx->cache = cache;

	redis_smart = redisConnect(master_gr_ip, master_gr_port);
	if ((redis_smart == NULL) || (redis_smart->err != 0))
	{
		errcode = E_REDIS_MASTER_FILTER_SMART;
		goto exit;
	}
	ctx->redis_smart = redis_smart;

	redis_relation_advcat = redisConnect(slave_advcat_ip, slave_advcat_port);
	if ((redis_relation_advcat == NULL) || (redis_relation_advcat->err != 0))
	{
		errcode = E_REDIS_ADVCAT;
		goto exit;
	}
	ctx->redis_relation_advcat = redis_relation_advcat;

	for (int i = 0; i < thread_count; i++)
	{
		REDIS_FDS redis_fds;

		redisContext *redis_id = NULL;
		redisContext *redis_wblist_detail = NULL;
		redisContext *redis_gr = NULL;
		redisContext *redis_ur = NULL;
		redisContext *redis_user_advcat = NULL;

		errcode = E_SUCCESS;

		redis_id = redisConnect(master_filter_id_ip, master_filter_id_port);
		if ((redis_id == NULL) || (redis_id->err != 0))
		{
			errcode = E_REDIS_MASTER_FILTER_ID;
			goto exit;
		}

		redis_wblist_detail = redisConnect(slave_wblist_ip, slave_wblist_port);

		if ((redis_wblist_detail == NULL) || (redis_wblist_detail->err != 0))
		{
			errcode = E_REDIS_SLAVE_WBLIST;
			goto redis_proc;
		}

		redis_gr = redisConnect(slave_gr_ip, slave_gr_port);
		if ((redis_gr == NULL) || (redis_gr->err != 0))
		{
			errcode = E_REDIS_SLAVE_GR;
			goto redis_proc;
		}

		redis_ur = redisConnect(slave_ur_ip, slave_ur_port);
		if ((redis_ur == NULL) || (redis_ur->err != 0))
		{
			errcode = E_REDIS_SLAVE_UR;
			goto redis_proc;
		}

		redis_user_advcat = redisConnect(slave_advcat_ip, slave_advcat_port);
		if ((redis_user_advcat == NULL) || (redis_user_advcat->err != 0))
		{
			errcode = E_REDIS_ADVCAT;
			goto redis_proc;
		}

redis_proc:
		if (errcode != E_SUCCESS)
		{
			if (redis_id != NULL)
			{
				redisFree(redis_id);
				redis_id = NULL;
			}

			if (redis_wblist_detail != NULL)
			{
				redisFree(redis_wblist_detail);
				redis_wblist_detail = NULL;
			}

			if (redis_gr != NULL)
			{
				redisFree(redis_gr);
				redis_gr = NULL;
			}

			if (redis_ur != NULL)
			{
				redisFree(redis_ur);
				redis_ur = NULL;
			}

			if (redis_user_advcat != NULL)
			{
				redisFree(redis_user_advcat);
				redis_user_advcat = NULL;
			}

			goto exit;
		}

		redis_fds.redis_id = redis_id;
		redis_fds.redis_wblist_detail = redis_wblist_detail;
		redis_fds.redis_gr = redis_gr;
		redis_fds.redis_ur = redis_ur;
		redis_fds.redis_user_advcat = redis_user_advcat;

		ctx->redis_fds.push_back(redis_fds);
	}

	pthread_mutex_init(&ctx->v_cmd_smart_mutex, 0);
	thrad_value_smart = pthread_create(&ctx->t_pipeline_smart, NULL, thread_redis_pipeline_smart, (void *)ctx);
	if (thrad_value_smart != 0)
	{
		errcode = E_CREATETHREAD;
		goto exit;
	}
	v_thread_id.push_back(ctx->t_pipeline_smart);

	pthread_mutex_init(&ctx->bid_info_mutex, 0);
	pthread_mutex_lock(&ctx->bid_info_mutex);
	ctx->zero_flag = 0;
	pthread_mutex_unlock(&ctx->bid_info_mutex);

	ctx->timer_recommend = 0;

	// 添加项目的出价比例
	get_semaphore(ctx->cache, &redis_info_addr, &sem_id);
	if(semaphore_release(sem_id))//+1
	{
		redis_cashe = (REDIS_INFO* )redis_info_addr;
		errcode = read_smart_ratio(ctx, redis_cashe);		//get smart ratio from redis

		if (errcode == E_SUCCESS) // 打印读取的比例值
		{
			pthread_mutex_lock(&ctx->bid_info_mutex);
			const map<string, map<int, BID_INFO> > &m_info = ctx->group_bid_info;
			print_map_bid_ratio_info(ctx->logid, LOGINFO, m_info);
			pthread_mutex_unlock(&ctx->bid_info_mutex);
		}
		else if(errcode == E_REDIS_CONNECT_INVALID)
		{
			goto exit;
		}
	}
	else
	{
		cflog(ctx->logid, LOGERROR, "%s, %s/%d, semaphore_release failed!", 
			"", __FUNCTION__, __LINE__);

		errcode = E_SEMAPHORE;
		goto exit;
	}
	if(semaphore_wait(sem_id))//-1
	{
		//		cflog(logid, LOGINFO, "%s, %s/%d, semaphore_wait OK!", 
		//			id, __FUNCTION__, __LINE__);
	}
	else
	{
		cflog(ctx->logid, LOGERROR, "%s, %s/%d, semaphore_wait failed!!", 
			"", __FUNCTION__, __LINE__);

		errcode = E_SEMAPHORE;
		goto exit;
	}

	ctx->pipe_err = E_SUCCESS;
	pthread_mutex_init(&ctx->pipe_err_mutex, 0);
	//get advcat relation info
	pthread_mutex_init(&ctx->advcat_mutex, 0);
	errcode = getADVCAT(ctx->redis_relation_advcat, ctx->logid, ctx->advcat);
#ifdef DEBUG
	{
		map<string, double>::const_iterator advcat_it = ctx->advcat.begin();
		while(advcat_it != ctx->advcat.end())
		{
			cout <<"first :" << advcat_it->first <<" , second :" <<advcat_it->second<<endl;
			++advcat_it;
		}

	}
#endif
	if(errcode != E_REDIS_CONNECT_INVALID)
	{
		errcode = E_SUCCESS;
	}

exit:
	if (slave_major_ip != NULL)
	{
		free(slave_major_ip);
		slave_major_ip = NULL;
	}
	if (master_filter_id_ip != NULL)
	{
		free(master_filter_id_ip);
		master_filter_id_ip = NULL;
	}
	if (slave_gr_ip != NULL)
	{
		free(slave_gr_ip);
		slave_gr_ip = NULL;
	}
	if (master_gr_ip != NULL)
	{
		free(master_gr_ip);
		master_gr_ip = NULL;
	}
	if (slave_ur_ip != NULL)
	{
		free(slave_ur_ip);
		slave_ur_ip = NULL;
	}
	if (pub_sub_ip != NULL)
	{
		free(pub_sub_ip);
		pub_sub_ip = NULL;
	}

	if (slave_wblist_ip != NULL)
	{
		free(slave_wblist_ip);
		slave_wblist_ip = NULL;
	}
	if (slave_advcat_ip != NULL)
	{
		free(slave_advcat_ip);
		slave_advcat_ip = NULL;
	}

	if (settle_server != NULL)
	{
		free(settle_server);
		settle_server = NULL;
	}
	if (click_server != NULL)
	{
		free(click_server);
		click_server = NULL;
	}
	if (impression_server != NULL)
	{
		free(impression_server);
		impression_server = NULL;
	}
	if (active_server != NULL)
	{
		free(active_server);
		active_server = NULL;
	}

	if (settle_https_server != NULL)
	{
		free(settle_https_server);
		settle_https_server = NULL;
	}
	if (click_https_server != NULL)
	{
		free(click_https_server);
		click_https_server = NULL;
	}
	if (impression_https_server != NULL)
	{
		free(impression_https_server);
		impression_https_server = NULL;
	}
	if (active_https_server != NULL)
	{
		free(active_https_server);
		active_https_server = NULL;
	}
	if (errcode == E_SUCCESS)
	{
		*pctx = (uint64_t)ctx;
	}
	else
	{
		cflog(logid, LOGERROR, "at fn:%s, ln:%d, errcode:0x%x", __FUNCTION__, __LINE__, errcode);

		if (thrad_value_smart != 0)
		{
			pthread_kill(ctx->t_pipeline_smart, SIGKILL);	
		}
		bid_filter_uninitialize((uint64)ctx);
	}

	cflog(logid, LOGINFO, "at fn:%s, ln:%d", __FUNCTION__, __LINE__);

	return errcode;
}

int bid_filter_uninitialize(IN uint64_t ctx)
{
	if (ctx != 0)
	{
		FILTERCONTEXT *fctx = (FILTERCONTEXT*)ctx;

		if (fctx->cache != 0)
		{
			uninit_redis_operation(fctx->cache);
			fctx->cache = 0;
		}

		if (fctx->redis_smart != NULL)
		{
			redisFree(fctx->redis_smart);
			fctx->redis_smart = NULL;
		}

		if(fctx->redis_relation_advcat != NULL)
		{
			redisFree(fctx->redis_relation_advcat);
			fctx->redis_relation_advcat = NULL;
		}

		for (int j = 0; j < fctx->redis_fds.size(); j++)
		{
			REDIS_FDS &fds = fctx->redis_fds[j];

			if (fds.redis_id != NULL)
			{
				redisFree(fds.redis_id);
				fds.redis_id = NULL;
			}

			if (fds.redis_wblist_detail != NULL)
			{
				redisFree(fds.redis_wblist_detail);
				fds.redis_wblist_detail = NULL;
			}

			if (fds.redis_gr != NULL)
			{
				redisFree(fds.redis_gr);
				fds.redis_gr = NULL;
			}

			if (fds.redis_ur != NULL)
			{
				redisFree(fds.redis_ur);
				fds.redis_ur = NULL;
			}

			if(fds.redis_user_advcat != NULL)
			{
				redisFree(fds.redis_user_advcat);
				fds.redis_user_advcat = NULL;
			}
		}
		fctx->redis_fds.clear();

		pthread_mutex_destroy(&fctx->v_cmd_smart_mutex);

		pthread_mutex_destroy(&fctx->pipe_err_mutex);

		pthread_mutex_destroy(&fctx->advcat_mutex);

		delete fctx;

		return E_SUCCESS;
	}

	return E_INVALID_PARAM_CTX;
}

int bid_time_clear(IN uint64_t ctx)
{
	if (ctx != 0)
	{
		FILTERCONTEXT *fctx = (FILTERCONTEXT*)ctx;

		pthread_mutex_lock(&fctx->bid_info_mutex);
		fctx->group_bid_info.clear();
		pthread_mutex_unlock(&fctx->bid_info_mutex);
		//     va_cout("clear success!");
		cflog(fctx->logid, LOGDEBUG, "clear success!");
		return E_SUCCESS;
	}

	return E_INVALID_PARAM_CTX;
}
