#include <stdlib.h>
#include "redisoperation.h"
#include <unistd.h>
#include <string.h>
#include "../common/errorcode.h"
#include "../common/setlog.h"

//#define SERVER_LOCAL "127.0.0.1"
//#define REDIS_PORT 7389

typedef struct redis_return_data
{
	sem_t extsemid;
	REDIS_INFO *extaddr;
	pthread_mutex_t g_mutex_data;
	sem_t semidarr[2];
	REDIS_INFO *addr[2];
	redisContext *g_context;
	redisContext *pubsub;
	uint64_t logid_local;
	uint8_t adx;
	pthread_t tid;
	string slave_ip;
	int slave_port;
	bool *run_flag;
}REDSI_RETURN_DATA;

template<typename T1>
void printVectorInt(vector<T1> &v)
{
	int i = 0;
	for(; i < v.size(); i++)
		cout << (int)v[i] << endl;
}

void printVectorString(vector<string> &v)
{
	int i = 0;
	for(; i < v.size(); i++)
		cout << v[i] << endl;
}

template<typename T2>
void printSetInt(set<T2> &v)
{
	typename set<T2>::iterator it;
	for(it=v.begin();it!=v.end();it++) 
		cout<<(int)*it <<endl;   
}

void printStringSet(set<string> &v)
{
	set<string>::iterator it;
	for(it=v.begin();it!=v.end();it++) 
		cout<< *it <<endl;   
}

void get_semaphore(IN uint64_t ctx, OUT uint64_t *cache, OUT sem_t *semid)
{
	REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)ctx;
	va_cout("addr is %p", redisdata);
	pthread_mutex_lock(&redisdata->g_mutex_data);
	*cache = (uint64_t)redisdata->extaddr;
	*semid = redisdata->extsemid;
	pthread_mutex_unlock(&redisdata->g_mutex_data);
}

void clearStruct(REDIS_INFO *redis_info)
{
	redis_info->groupid.clear();
	redis_info->groupinfo.clear();
	redis_info->wgroupinfo.clear();
	redis_info->creativeinfo.clear();
	redis_info->fc.clear();
	redis_info->group_target.clear();
	clear_ADX_TARGET_object(redis_info->adx_target);
	redis_info->wblist.clear();
	redis_info->wbdetaillist.clear();
	clear_ADXTEMPLATE(redis_info->adxtemplate);
	redis_info->img_imptype.clear();
	redis_info->grsmartbidinfo.clear();
	init_LOG_CONTROL_INFO(redis_info->logctrlinfo);
}

void fillwbListDetail(redisContext *g_context, uint64_t logid_local, REDIS_INFO *redis_info, const char *groupid, const char *kind)
{
	redisReply *reply;
	set<string> tmp;
	pair<map<string, set<string> >::iterator, bool> ret;
	//va_cout("groupid is %s and  kind is %s",groupid, kind);

	reply = (redisReply *)redisCommand(g_context, "SMEMBERS %s_%s", kind, groupid);
	if (reply == NULL)
	{
		//va_cout("redis error %s\n", g_context->errstr);
		redis_info->errorcode = E_REDIS_CONNECT_INVALID;
		return;
	}
	
	int i = 0;
	if(reply->type == REDIS_REPLY_ARRAY && reply->elements)
	{
		while(i < reply->elements)
		{
			tmp.insert(reply->element[i]->str);
			i++;
		}
		char tmpgroupid[64] = "";
		sprintf(tmpgroupid, "%s_%s", kind, groupid);
		ret = redis_info->wbdetaillist.insert(make_pair(string(tmpgroupid), tmp));

		if(!ret.second)
		{
			va_cout("the key :%s has already exist\n", tmpgroupid);
		}
	}
	freeReplyObject(reply);
}

void fillwbList(redisContext *g_context, uint64_t logid_local, REDIS_INFO *redis_info, char *groupid)
{
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "idfa_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "androidid_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "imei_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "mac_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "app_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "idfa_sha1_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "androidid_sha1_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "imei_sha1_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "mac_sha1_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "idfa_md5_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "androidid_md5_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "imei_md5_wl");
	fillwbListDetail(g_context, logid_local, redis_info, groupid, "mac_md5_wl");
}

bool  fillADXinfo(redisContext *g_context, uint64_t logid_local, REDIS_INFO *redis_info)
{
	/*redisReply *reply;
	set<string> tmp;
	pair<map<string, set<string> >::iterator, bool> ret;

	reply = (redisReply *)redisCommand(g_context, "SMEMBERS adx_templates");
	if (reply == NULL)
	{
		va_cout("redis error %s\n", g_context->errstr);
		return false;
	}

	int i = 0;
	if(reply->type == REDIS_REPLY_ARRAY && reply->elements)
	{
		while(i < reply->elements)
		{
			json_t *root = NULL, *label;
			ADXTEMPLATE adxinfo;
			cout << reply->element[i]->str <<endl;
			json_parse_document(&root, reply->element[i]->str);
			if (root == NULL)
			{
				cout<<"parse adxinfo:" << reply->element[i]->str << " failer" << endl;
				goto next_loop;
			}

			init_ADXTEMPLATE(adxinfo);
			if(JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_NUMBER))
				adxinfo.adx = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(root, label, "iurl", JSON_ARRAY))
			    jsonGetStringArray(label, adxinfo.iurl);
			if(JSON_FIND_LABEL_AND_CHECK(root, label, "cturl", JSON_ARRAY))
				jsonGetStringArray(label, adxinfo.cturl);
			if(JSON_FIND_LABEL_AND_CHECK(root, label, "aurl", JSON_STRING))
				adxinfo.aurl = label->child->text;
			if(JSON_FIND_LABEL_AND_CHECK(root, label, "nurl", JSON_STRING))
				adxinfo.nurl = label->child->text;
			if(JSON_FIND_LABEL_AND_CHECK(root, label, "adms", JSON_ARRAY))
			{
				json_t *sectmp;
				sectmp = label->child->child;
				while(sectmp)
				{
					ADMS adms;
					init_ADMS(adms);
					if(JSON_FIND_LABEL_AND_CHECK(sectmp, label, "os", JSON_NUMBER))
						adms.os = atoi(label->child->text);
					if(JSON_FIND_LABEL_AND_CHECK(sectmp, label, "type", JSON_NUMBER))
						adms.type = atoi(label->child->text);
					if(JSON_FIND_LABEL_AND_CHECK(sectmp, label, "type", JSON_STRING))
						adms.adm = label->child->text;
					sectmp =sectmp->next;
					adxinfo.adms.push_back(adms);
				}

			}
			json_free_value(&root);
next_loop:
			i++;	
		}
	}
	else
		return false;

	freeReplyObject(reply);*/

	return true;
}

int parseADXINFO(redisContext *g_context, uint8_t adx, uint64_t logid_local, ADXTEMPLATE &adxtemplate)
{
	int errorcode = E_SUCCESS;
	string adxinfo = "";
	json_t *root, *label;
	root = label = NULL;

	if(adx == ADX_MAX)
		goto exit;

	errorcode = getRedisValue(g_context, logid_local, "adx_templates", adxinfo);
	if (errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}
	if(adxinfo.size() == 0)
	{
		writeLog(logid_local, LOGERROR, "adx_templates does not exist!");
		errorcode = E_REDIS_NO_CONTENT;
		goto exit;
	}

	json_parse_document(&root, adxinfo.c_str());
	if (root == NULL)
	{
		va_cout("parse adxinfo: %s failed", adxinfo.c_str());
		errorcode = E_INVALID_PARAM_JSON;
		goto exit;
	}
	//va_cout("adxinfo : %s", adxinfo.c_str());
	errorcode = E_INVALID_PARAM_ADX;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "templates", JSON_ARRAY))
	{
		json_t *tmp;
		tmp = label->child->child;
		while(tmp)
		{
			if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "adx", JSON_NUMBER))
			{
				uint8_t redisadx = atoi(label->child->text);
				if(redisadx == adx)
				{
					adxtemplate.adx = adx;
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "ratio", JSON_NUMBER))
						adxtemplate.ratio = atof(label->child->text);
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "iurl", JSON_ARRAY))
						jsonGetStringArray(label, adxtemplate.iurl);
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "cturl", JSON_ARRAY))
						jsonGetStringArray(label, adxtemplate.cturl);
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "aurl", JSON_STRING))
						adxtemplate.aurl = label->child->text;
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "nurl", JSON_STRING))
						adxtemplate.nurl = label->child->text;
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "adms", JSON_ARRAY))
					{
						json_t *sectmp;
						sectmp = label->child->child;
						while(sectmp)
						{
							ADMS adms;
							init_ADMS(adms);
							if(JSON_FIND_LABEL_AND_CHECK(sectmp, label, "os", JSON_NUMBER))
								adms.os = atoi(label->child->text);
							if(JSON_FIND_LABEL_AND_CHECK(sectmp, label, "type", JSON_NUMBER))
								adms.type = atoi(label->child->text );
							if(JSON_FIND_LABEL_AND_CHECK(sectmp, label, "adm", JSON_STRING))
								adms.adm = label->child->text;
							sectmp =sectmp->next;
							adxtemplate.adms.push_back(adms);
						}
					}
					errorcode = E_SUCCESS;
					goto exit;
				}
			}
			tmp = tmp->next;
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);

	return errorcode;
}

int addGroup(redisContext *g_context, uint8_t adx, uint64_t logid_local, REDIS_INFO *redis_info, string groupid, string groupid_type)
{
	int errorcode = E_SUCCESS;
	GROUPINFO ginfo;

	init_GROUPINFO_object(ginfo);
	errorcode = getGroupInfo(g_context, adx, logid_local, groupid, ginfo);
	if(errorcode == E_SUCCESS)
	{
		pair<map<string, vector<string> >::iterator, bool> ret;
		vector<string> mapidvec;
		string mapids = "";
		json_t *grouproot, *grouplabel;
		grouproot = grouplabel = NULL;

		GROUP_TARGET group_target;
		FREQUENCYCAPPINGRESTRICTION frec;
		WBLIST wblist;
		GROUP_SMART_BID_INFO grsmartbidinfo;

		errorcode = getRedisValue(g_context, logid_local, "dsp_groupid_mapids_" + groupid, mapids);
		if (errorcode == E_REDIS_CONNECT_INVALID)
		{
			goto exit;
		}

		if(mapids.size() == 0)
		{
			writeLog(logid_local, LOGINFO, "dsp_groupid_mapids_" + groupid + " does not exist!");
			errorcode = E_REDIS_NO_CREATIVE;
			goto exit;
		}

		json_parse_document(&grouproot, mapids.c_str());
		if(grouproot == NULL)
		{
			va_cout("parse dsp_groupid_mapids failer\n");
			errorcode = E_INVALID_PARAM_JSON;
			goto exit;
		}

		if(JSON_FIND_LABEL_AND_CHECK(grouproot, grouplabel, "mapids", JSON_ARRAY))
		{
			json_t *grouptemp  = NULL;
			if (grouplabel->child == NULL || grouplabel->child->child == NULL)
			{
				va_cout("groupid: %s's mapids value is NULL", groupid.c_str());
				errorcode = E_REDIS_NO_CREATIVE;
				goto exit;
			}
			grouptemp = grouplabel->child->child;//the iterator of mapid list

			while (grouptemp != NULL)//mapid list
			{
				//va_cout("mapid is %s", grouptemp->text.c_str());

				//根据mapid获取mapid详细信息
				if((grouptemp->type == JSON_STRING) && (strlen(grouptemp->text) > 0))
				{
					//va_cout("mapidinfo : %s", mapidinfo.c_tr());
					CREATIVEOBJECT creative;
					init_CREATIVE_object(creative);
					creative.groupid_type = groupid_type;
					
					errorcode = getCreativeInfo(g_context, adx, logid_local, redis_info->img_imptype, string(grouptemp->text), creative);
					if(errorcode == E_SUCCESS)
					{
						creative.advcat = ginfo.advcat;
						creative.dealid = ginfo.dealid;

						//当mapid详细信息存在时插入mapidvec
						mapidvec.push_back(string(grouptemp->text));
						redis_info->creativeinfo.insert(make_pair(string(grouptemp->text), creative));
					}
					else if (errorcode == E_REDIS_CONNECT_INVALID)
					{
						goto exit;
					}
				}
				grouptemp = grouptemp->next;
			}
		}

		if(mapidvec.size() == 0)
		{
			writeLog(logid_local, LOGINFO, "groupid: %s's no valid mapid", groupid.c_str());
			errorcode = E_REDIS_NO_CREATIVE;
			goto exit;
		}

		ret = redis_info->groupid.insert(make_pair(groupid, mapidvec));

		if(!ret.second)
		{
			writeLog(logid_local, LOGINFO, "the key : %s has already exist", groupid.c_str());
		}

		init_GROUP_TARGET_object(group_target);
		init_FC_object(frec);
		init_WBLIST_object(wblist);
		init_GROUP_SMART_BID(grsmartbidinfo);

		errorcode = getGroupSmartBidInfo(g_context, adx, logid_local, groupid, grsmartbidinfo);
		if(errorcode == E_SUCCESS)
			redis_info->grsmartbidinfo.insert(make_pair(groupid, grsmartbidinfo));
		else if (errorcode == E_REDIS_CONNECT_INVALID)
			goto exit;

		errorcode = get_GROUP_TARGET_object(g_context, adx, logid_local, groupid, group_target);
		if(errorcode == E_SUCCESS)
			redis_info->group_target.insert(make_pair(groupid, group_target));
		else if (errorcode == E_REDIS_CONNECT_INVALID)
			goto exit;

		errorcode = getFrequencyCappingRestriction(g_context, adx, logid_local, groupid.c_str(), frec);
		if(errorcode == E_SUCCESS)
		{
			redis_info->fc.insert(make_pair(groupid, frec));
		}
		else if (errorcode == E_REDIS_CONNECT_INVALID)
			goto exit;

		if(errorcode != E_REDIS_GROUP_FC_INVALID)
		{
			errorcode = getWBList(g_context, logid_local, groupid, wblist);
			if(errorcode == E_SUCCESS)
			{
				redis_info->wblist.insert(make_pair(groupid, wblist));
				//插入wgroupinfo
				redis_info->wgroupinfo.push_back(make_pair(groupid, ginfo));
			}
			else if (errorcode == E_REDIS_NO_CONTENT || errorcode == E_REDIS_KEY_NOT_EXIST)//插入groupinfo
			{
				redis_info->groupinfo.push_back(make_pair(groupid, ginfo));
			}
			else if (errorcode == E_REDIS_CONNECT_INVALID)
				goto exit;
		}
exit:
		if(grouproot != NULL)
			json_free_value(&grouproot);
	}

	return errorcode;
}

int get_ALL_GROUP_info(redisContext *g_context, uint8_t adx, uint64_t logid_local, REDIS_INFO *redis_info)
{
	int errorcode = E_SUCCESS;
	string dsp_groupids = "";
	string groupid_type = "dsp";
	json_t *dsp_root, *dsp_label;
	dsp_root = dsp_label = NULL;
	
	errorcode = getRedisValue(g_context, logid_local, "dsp_groupids", dsp_groupids);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto pap;
	}
	//没有在投项目可能就没有dsp_groupids的key
	if (dsp_groupids.size() == 0)
	{
		writeLog(logid_local, LOGINFO, "dsp_groupids does not exist!");
		errorcode = E_SUCCESS;
		goto pap;
	}
	//va_cout("dsp_groupids %s",  dsp_groupids.c_str());
	json_parse_document(&dsp_root, dsp_groupids.c_str());
	if (dsp_root == NULL)
	{
		va_cout("parse dsp_groupids failer");
		errorcode = E_INVALID_PARAM_JSON;
		goto pap;
	}
	if(JSON_FIND_LABEL_AND_CHECK(dsp_root, dsp_label, "groupids", JSON_ARRAY))
	{
		json_t *temp;
		temp = dsp_label->child->child;//the iterator of groupid list
		while(temp != NULL)
		{
			if((temp->type == JSON_STRING) && (strlen(temp->text) > 0))
			{
				errorcode = addGroup(g_context, adx, logid_local, redis_info, string(temp->text), groupid_type);
				if(errorcode == E_REDIS_CONNECT_INVALID)
					goto pap;
			}
			temp = temp->next;
		}
	}

pap:
    string pap_groupids = "";
    json_t *pap_root, *pap_label;
    pap_root = pap_label = NULL;
    groupid_type = "pap";

    errorcode = getRedisValue(g_context, logid_local, "pap_groupids", pap_groupids);
    if(errorcode == E_REDIS_CONNECT_INVALID)
    {
        goto exit;
    }
    //va_cout("pap_groupids %s",  pap_groupids.c_str());
    json_parse_document(&pap_root, pap_groupids.c_str());
    if (pap_root == NULL)
    {
        va_cout("parse pap_groupids failer");
        errorcode = E_INVALID_PARAM_JSON;
        goto exit;
    }
    if(JSON_FIND_LABEL_AND_CHECK(pap_root, pap_label, "groupids", JSON_ARRAY))
    {
        json_t *temp;
        temp = pap_label->child->child;//the iterator of groupid list
        while(temp != NULL)
        {
            if((temp->type == JSON_STRING) && (strlen(temp->text) > 0))
            {
                errorcode = addGroup(g_context, adx, logid_local, redis_info, string(temp->text), groupid_type);
                if(errorcode == E_REDIS_CONNECT_INVALID)
                    goto exit;
            }
            temp = temp->next;
        }
    }
	
exit:
	if(dsp_root != NULL)
        json_free_value(&dsp_root);
    if(pap_root != NULL)
        json_free_value(&pap_root);
	return errorcode;
}

int parseImgImpSize(redisContext *g_context, uint8_t adx, uint64_t logid_local, map<string, uint8_t> &img_imp_size)
{
	int errcode = E_SUCCESS;
	string sizeinfo;
	json_t *root, *label;

	root = label= NULL;
	if(adx == ADX_MAX)
	{
		goto exit;
	}

	errcode = getRedisValue(g_context, logid_local, "dsp_img_imp_type", sizeinfo);
	if(errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if(sizeinfo.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "sizeinfo does not exist!");
		goto exit;
	}

	json_parse_document(&root, sizeinfo.c_str());
	if(root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "img_imp_type", JSON_ARRAY))
	{
		json_t *temp = label->child->child;
		json_t *sizeLabel;
		while(temp != NULL)
		{
			if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "adx", JSON_NUMBER))
			{
				int redis_adx = atoi(sizeLabel->child->text);
				if(redis_adx  == adx)
				{
					if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "imps", JSON_ARRAY))
					{
						json_t *sectmp;
						sectmp = sizeLabel->child->child;
						while(sectmp)
						{
							if(JSON_FIND_LABEL_AND_CHECK(sectmp, sizeLabel, "type", JSON_NUMBER))
							{
								uint8_t type = atoi(sizeLabel->child->text);
								if(JSON_FIND_LABEL_AND_CHECK(sectmp, sizeLabel, "sizes", JSON_ARRAY))
								{
									json_t *tempinner = sizeLabel->child->child;

									while (tempinner != NULL)
									{
										if(tempinner->type == JSON_STRING)
										{
											img_imp_size.insert(make_pair(tempinner->text, type));
										}
										tempinner = tempinner->next;
									}
								}
							}
							sectmp =sectmp->next;
						}
					}
				}
			}
			temp = temp->next;
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

int get_LOG_CONTOL_INFO(redisContext *context, uint8_t adx, uint64_t logid_local, LOG_CONTROL_INFO &logctrlinfo)
{
	int  errcode = E_SUCCESS;
	string loginfo = "";
	json_t *root, *label;

	root = label= NULL;
	if(adx == ADX_MAX)
	{
		goto exit;
	}

	errcode = getRedisValue(context, logid_local, "dsp_log_conf", loginfo);
	if(errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if(loginfo.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "loginfo does not exist!");
		goto exit;
	}

	json_parse_document(&root, loginfo.c_str());
	if(root == NULL)
	{
		writeLog(logid_local, LOGINFO, "loginfo is invalid json!");
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "dsp_log_conf", JSON_ARRAY))
	{
		json_t *temp = label->child->child;
		json_t *sizeLabel;
		while(temp != NULL)
		{
			if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "adx", JSON_NUMBER))
			{
				int redis_adx = atoi(sizeLabel->child->text);
				if(redis_adx  == adx)
				{
					if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "ferr_level", JSON_NUMBER))
						logctrlinfo.ferr_level = atoi(sizeLabel->child->text);
					if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "is_print_time", JSON_NUMBER))
						logctrlinfo.is_print_time = atoi(sizeLabel->child->text);
					goto exit;
				}
			}
			temp = temp->next;
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);

	return errcode;
}

int initializeStruct(redisContext *g_context, uint8_t adx, uint64_t logid_local, REDIS_INFO *redis_info)
{
	//pair<map<string, vector<string> >::iterator, bool> ret;
	int errorcode = E_SUCCESS;

	clearStruct(redis_info);
	//缓存平台相关信息
	/*if(!fillADXinfo(g_context, logid_local, redis_info))
		return;*/
	errorcode = parseADXINFO(g_context, adx, logid_local, redis_info->adxtemplate);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	errorcode = parseImgImpSize(g_context, adx, logid_local, redis_info->img_imptype);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	errorcode = get_ADX_TARGET_object(g_context, adx, logid_local, redis_info->adx_target);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	errorcode = get_LOG_CONTOL_INFO(g_context, adx, logid_local, redis_info->logctrlinfo);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	errorcode = get_ALL_GROUP_info(g_context, adx, logid_local, redis_info);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	errorcode = E_SUCCESS;
exit:
	redis_info->errorcode = errorcode;
	return errorcode;
}

void *major_thread(void *arg)
{
	REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)arg;
	uint32_t choose = 0;
	uint32_t next = 0;
//	pthread_detach(pthread_self());
	int errcode = E_SUCCESS;
	//redisReply *reply;
	//const char* command = "PSUBSCRIBE dsp_news";  
	/*redisContext *context;
 	context = redisConnect("127.0.0.1", 8389);
	if (context->err)
	{
		redisFree(g_context);
		cout << "Execute redisConnect" << string(SERVER_LOCAL) <<"failed"<<endl;
		return NULL;
	}*/
	/*reply = (redisReply*)redisCommand(redisdata->pubsub, command);  
    if(context->err != 0)
	{
		 cout <<"execute command "<<command <<"failed" << endl;
	}
	else
		cout <<"execute subscribe success " << command << endl;
			   
	freeReplyObject(reply);
	while(redisGetReply(context, (void **)&reply) == REDIS_OK)
	*/
	while(*redisdata->run_flag)
	{
		//printf("ready to fresh\n");
		next = choose + 1;
		if (redisdata->g_context == NULL)  //reconnect
		{
		   redisdata->g_context = redisConnect(redisdata->slave_ip.c_str(), redisdata->slave_port);
		   if (redisdata->g_context && redisdata->g_context->err)
		   {
			   writeLog(redisdata->logid_local, LOGDEBUG, "cache reconnect err,err:%s", redisdata->g_context->errstr);
			   redisFree(redisdata->g_context);
			   redisdata->g_context = NULL;
		   }
		}

		if (redisdata->g_context != NULL)
		{
			errcode = initializeStruct(redisdata->g_context, redisdata->adx, redisdata->logid_local, redisdata->addr[next %2]);

			if (redisdata->adx == ADX_MAX && errcode == E_REDIS_CONNECT_INVALID)   
			{
				writeLog(redisdata->logid_local, LOGDEBUG, "initializeStruct context is invaild!");
				redisFree(redisdata->g_context);
				redisdata->g_context = NULL;
			}
		}

		pthread_mutex_lock(&redisdata->g_mutex_data);
		redisdata->extsemid = redisdata->semidarr[next %2];
		redisdata->extaddr = redisdata->addr[next%2];
		pthread_mutex_unlock(&redisdata->g_mutex_data);
		sleep(5);
		if(semaphore_test(redisdata->semidarr[choose%2]))
		{
			choose++;
			//printf("no refreance\n");
		}
		//freeReplyObject(reply);
	}
	//pthread_mutex_destroy(&redisdata->g_mutex_data);
	cout<<"cache major thread exit"<<endl;
	return NULL;
}

void myprintfcreative(CREATIVEOBJECT *creativeinfo)
{
	printf("mapid: %s\n", creativeinfo->mapid.c_str());
	printf("groupid: %s\n", creativeinfo->groupid.c_str());
	printf("price: %lf\n", creativeinfo->price);
	printf("adid: %s\n", creativeinfo->adid.c_str());
	printf("type: %u\n", creativeinfo->type);
	printf("ftype: %u\n", creativeinfo->ftype);
	printf("ctype: %u\n", creativeinfo->ctype);
	printf("bundle: %s\n", creativeinfo->bundle.c_str());
	printf("apkname: %s\n", creativeinfo->apkname.c_str());
	printf("w: %u\n", creativeinfo->w);
	printf("h: %u\n", creativeinfo->h);
	printf("curl: %s\n", creativeinfo->curl.c_str());
	cout << "cmonitorurl" << endl;
	printVectorString(creativeinfo->cmonitorurl);
	cout << "imonitorurl" << endl;	
	printVectorString(creativeinfo->imonitorurl);
	printf("monitorcode: %s\n", creativeinfo->monitorcode.c_str());
	printf("sourceurl: %s\n", creativeinfo->sourceurl.c_str());
	printf("cid: %s\n", creativeinfo->cid.c_str());
	printf("crid: %s\n", creativeinfo->crid.c_str());
	printSetInt(creativeinfo->attr);
	cout <<"badx :" <<endl;
	printSetInt(creativeinfo->badx);
	printf("icon detail:\n");
	printf("ftype: %d, w: %d, h: %d and sourceurl: %s\n", creativeinfo->icon.ftype, creativeinfo->icon.w, creativeinfo->icon.h, creativeinfo->icon.sourceurl.c_str());
	printf("title: %s\n", creativeinfo->title.c_str());
	printf("description: %s\n", creativeinfo->description.c_str());
	printf("rating %d\n", creativeinfo->rating);
	printf("cattext: %s\n", creativeinfo->ctatext.c_str());
	printf("exts:\n");
	cout <<" exts: id :" << creativeinfo->ext.id << " ext :" <<creativeinfo->ext.ext <<endl; 
	return;
}

void myprintf_APP_TARGET(APP_TARGET app_target)
{
	printf("cat flag: %d\n", app_target.cat.flag);
	printf("cat wlist:");
	printSetInt(app_target.cat.wlist);
	printf("cat blist:");
	printSetInt(app_target.cat.blist);
	printf("app flag: %d\n", app_target.id.flag);
	printf("app wlist:");
	printStringSet(app_target.id.wlist);
	printf("\napp blist:");
	printStringSet(app_target.id.blist);
}

void myprintf_DEVICE_TARGET(DEVICE_TARGET dev_target)
{
	printf("flag: %d\n", dev_target.flag);
	printf("regioncode:\n");
	printSetInt(dev_target.regioncode);
	printf("connectiontype:\n");
	printSetInt(dev_target.connectiontype);
	printf("os:\n");
	printSetInt(dev_target.os);
	printf("carrier:\n");
	printSetInt(dev_target.carrier);
	printf("devicetype:\n");
	printSetInt(dev_target.devicetype);
	printf("make:\n");
	printSetInt(dev_target.make);
}

void myprintf_SCENE_TARGET(SCENE_TARGET scene_target)
{
	printf("flag: %d\n", scene_target.flag);
	printf("loc:\n");
	printStringSet(scene_target.loc_set);
}

void myprintfgrptarget(GROUP_TARGET target)
{
	printf("groupid: %s\n", target.groupid.c_str());
	printf("dev target\n");
	myprintf_DEVICE_TARGET(target.device_target);
	printf("app target\n");
	myprintf_APP_TARGET(target.app_target);
	printf("scene target\n");
	myprintf_SCENE_TARGET(target.scene_target);
	
	return;
}

void myprintfadxtarget(ADX_TARGET target)
{
	printf("adx: %d\n", target.adx);
	printf("dev target\n");
	myprintf_DEVICE_TARGET(target.device_target);
	printf("app target\n");
	myprintf_APP_TARGET(target.app_target);

	return;
}

void myprintffc(FREQUENCYCAPPINGRESTRICTION fc)
{
	printf("groupid: %s\n", fc.groupid.c_str());
	printf("group frequency:\n");
	map<uint8_t, FREQUENCY_RESTRICTION_GROUP>::const_iterator map_it_t = fc.frg.begin();
	while(map_it_t != fc.frg.end())
	{
		FREQUENCY_RESTRICTION_GROUP frg = map_it_t->second;
		printf("adx: %d\n", map_it_t->first);
		printf("type: %u\n", frg.type);
		printf("period: %u\n", frg.period);
		printf("frequency:\n");
		printVectorInt(frg.frequency);
		++map_it_t;
	}

	printf("user frequency :\n");
	printf("type: %u\n", fc.fru.type);
	printf("period: %u\n", fc.fru.period);
	map<string, uint32_t>::const_iterator map_it = fc.fru.capping.begin();
	while(map_it != fc.fru.capping.end())
	{
		cout <<"mapid :" << map_it->first <<endl;
		cout <<"frequency :" << map_it->second <<endl;
		map_it++;
	}
	return;
}

void myprintfwblist(WBLIST wblist)
{
	cout <<"groupid:" <<wblist.groupid <<endl;
	cout <<"relationid:" <<wblist.relationid <<endl;
	cout <<"ratio:"<< wblist.ratio <<endl;
	cout <<"mprice:" <<wblist.mprice <<endl;
	return;
}

void  myprintfwbdetail(set<string> &v)
{
	set<string>::iterator it; 
	for(it = v.begin();it != v.end();it++) 				
	{
		cout<<*it<<endl;    
	}
	return;
}

void myprintfadxinfo(ADXTEMPLATE adxinfo)
{
	printVectorString(adxinfo.cturl);
	printVectorString(adxinfo.iurl);
	//cout<<"ratio :"<<setiosflags(ios::fixed)<<setprecision(7)<<adxinfo.ratio<<endl;
	cout <<"ratio :";
	cout.precision(7);
	cout.setf( ios::fixed );
	cout <<adxinfo.ratio<<endl;
	cout <<"aurl:" <<adxinfo.aurl << endl;
	cout <<"nurl:" << adxinfo.nurl << endl;
	int i = 0;
	for(i = 0; i < adxinfo.adms.size(); i++)
	{
		ADMS adm = adxinfo.adms[i];
		printf("os is %u,type is %u\n", adm.os, adm.type);
		cout <<"adm is:" << adm.adm <<endl;
	}
	return;
}

void myprintgroupinfo(GROUPINFO ginfo)
{
	cout <<"groupid cat :" <<endl;
	printSetInt(ginfo.cat);
	cout <<"advcat " << ginfo.advcat <<endl;
	cout <<"adomain " << ginfo.adomain <<endl;
	cout <<"groupid adx :" <<endl;
	printSetInt(ginfo.adx);
	cout <<"at" <<(int)ginfo.at <<endl;
	cout <<"redirect" <<(int)ginfo.redirect <<endl;
	cout <<"effectmonitor" <<(int)ginfo.effectmonitor <<endl;
	cout << "exts" << endl;
	cout <<" advid :" <<ginfo.ext.advid << endl;
}

void myprintgroupsmartinfo(GROUP_SMART_BID_INFO grsmartbidinfo)
{
	cout << " flag :" <<(int)grsmartbidinfo.flag <<endl;
	printf(" maxratio :%lf\n minratio :%lf\n interval :%d\n", grsmartbidinfo.maxratio, grsmartbidinfo.minratio, grsmartbidinfo.interval);
}

void parseStruct(uint64_t addr)
{
	va_cout("parseStruct\n");
	REDIS_INFO *redis =(REDIS_INFO *)addr;
	map<string, vector<string> >::const_iterator map_it  = redis->groupid.begin();
	while(map_it != redis->groupid.end())
	{
		vector<string> v_mapid = map_it->second;
		cout <<"groupid: " << map_it->first <<endl;
		if (v_mapid.size() == 0)
		{
			cout<<"groupid's mapid "<<" does not exist!"<<"requestid:"<<endl;
		}

		for (int i = 0; i < v_mapid.size(); i++)
		{
			cout<< "map[" << i << "] mapid: " << v_mapid[i] <<endl;
		}
		++map_it;
	}

	map<string, CREATIVEOBJECT>::iterator map_it_co = redis->creativeinfo.begin();
	while(map_it_co != redis->creativeinfo.end())
	{
		cout <<"mapid: " << map_it_co->first <<endl;
		CREATIVEOBJECT creativeinfo = map_it_co->second;
		myprintfcreative(&creativeinfo);
		++map_it_co;
	}

	map<string, FREQUENCYCAPPINGRESTRICTION>::iterator map_it_fc = redis->fc.begin();
	while(map_it_fc != redis->fc.end())
	{
		cout <<"fc: " << map_it_fc->first <<endl;
		FREQUENCYCAPPINGRESTRICTION fc = map_it_fc->second;
		myprintffc(fc);
		++map_it_fc;
	}

	map<string, GROUP_TARGET>::iterator map_it_t = redis->group_target.begin();
	while(map_it_t != redis->group_target.end())
	{
		cout <<"target: " << map_it_t->first <<endl;
		GROUP_TARGET target = map_it_t->second;
		myprintfgrptarget(target);
		++map_it_t;
	}

	map<string, WBLIST>::const_iterator map_it_wb  = redis->wblist.begin();
	while(map_it_wb != redis->wblist.end())
    {
		cout <<"wblist: " << map_it_wb->first <<endl;
		WBLIST wblist= map_it_wb->second;
		myprintfwblist(wblist);
		++map_it_wb;
    }
	
	map<string, uint8_t>::const_iterator map_it_img  = redis->img_imptype.begin();
	cout <<"size	type"<<endl; 
	while(map_it_img != redis->img_imptype.end())
	{
		cout << map_it_img->first << "	" << (int)map_it_img->second <<endl;
		++map_it_img;
	}

	myprintfadxinfo(redis->adxtemplate);
	myprintfadxtarget(redis->adx_target);

	cout <<"groupid: " <<endl;
	for(int i = 0; i < redis->groupinfo.size(); i++)
	{
		pair<string, GROUPINFO> it = redis->groupinfo[i];
		{
			cout <<"groupid: " << it.first <<endl;
			GROUPINFO ginfo = it.second;
			myprintgroupinfo(ginfo);
		}
	}

	cout <<"wgroupid: " <<endl;
	for(int i = 0; i < redis->wgroupinfo.size(); i++)
	{
		pair<string, GROUPINFO> it = redis->wgroupinfo[i];
		{
			cout <<"groupid: " << it.first <<endl;
			GROUPINFO ginfo = it.second;
			myprintgroupinfo(ginfo);
		}
	}

	cout <<"smart bid: " <<endl;
	map<string, GROUP_SMART_BID_INFO>::const_iterator map_it_gsmartbid  = redis->grsmartbidinfo.begin();
	while(map_it_gsmartbid != redis->grsmartbidinfo.end())
	{
		cout <<"groupid: " << map_it_gsmartbid->first << endl;
		myprintgroupsmartinfo(map_it_gsmartbid->second);
		++map_it_gsmartbid;
	}
	cout <<"log control info " <<endl;
	cout << "ferr_level is " <<(int)redis->logctrlinfo.ferr_level << " and is_print_time is " << (int)redis->logctrlinfo.is_print_time <<endl;
	return;
}

void *operate(void *arg)
{
//	pthread_detach(pthread_self());
	uint64_t data = (uint64_t)arg;
	uint64_t addr = 0;
	sem_t sem_id;
	//sleep(20);
	//int i = 4;
	while(1)
	//while(i)
	{
		
		get_semaphore(data, &addr, &sem_id);
		printf("thid is %u semid is %d	addr is %p\n", (unsigned)pthread_self(), sem_id, addr);
		if(semaphore_release(sem_id))
		{
		//	printf("wait to die\n");
			//sleep(10);
			parseStruct(addr);
			if(semaphore_wait(sem_id))
		//		;
				printf("wait ok\n");
		}
		//i--;
	}
	//printf("over\n");
}

int init_redis_operation(IN uint64_t logid_local, IN uint8_t adx, IN char *slave_ip, IN int slave_port, IN char *pubsub_ip, IN int pubsub_port, IN bool *run_flag, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx)
{
	if(adx == ADX_UNKNOWN)
		return E_INVALID_PARAM_ADX;

	int ret = E_SUCCESS;
	pthread_t threadid;
	REDSI_RETURN_DATA *redisdata = NULL;
	int res;
	redisdata = new REDSI_RETURN_DATA;
	if (redisdata == NULL)
	{
		ret = E_MALLOC;
		goto exit;
	}

	redisdata->g_context = NULL;
	redisdata->addr[0] = redisdata->addr[1] = NULL;

	redisdata->slave_ip = slave_ip;
	redisdata->slave_port = slave_port;
	redisdata->g_context = redisConnect(slave_ip, slave_port);
	if (redisdata->g_context->err)
	{
		va_cout("Execute redisConnect %s failed", slave_ip);
		ret = E_REDIS_SLAVE_MAJOR;
		goto exit;
	}

	if(!createsemaphore(redisdata->semidarr[0]))
	{
		ret = E_SEMAPHORE;
		goto exit;
	}
	if(!createsemaphore(redisdata->semidarr[1]))
	{
		ret = E_SEMAPHORE;
		goto exit;
	}

	//cout << "semid is " << redisdata->semidarr[0] << " and " << redisdata->semidarr[1] <<endl;

	redisdata->addr[0] = new REDIS_INFO;
	if(redisdata->addr[0] == NULL)
	{
		ret = E_MALLOC;
		goto exit;
	}
	redisdata->addr[1] = new REDIS_INFO;
	if(redisdata->addr[1] == NULL)
	{
		ret = E_MALLOC;
		goto exit;
	}
	//va_cout( "addr is %p and %p" , addr[0] ,addr[1]);
	pthread_mutex_init(&redisdata->g_mutex_data, 0);
	redisdata->logid_local = logid_local;
	redisdata->adx = adx;
	ret = initializeStruct(redisdata->g_context, redisdata->adx, redisdata->logid_local, redisdata->addr[0]);
	if(ret == E_REDIS_CONNECT_INVALID)
		goto exit;

	//parseStruct((uint64_t)redisdata->addr[0]);
	pthread_mutex_lock(&redisdata->g_mutex_data);
	redisdata->extsemid = redisdata->semidarr[0];
	redisdata->extaddr = redisdata->addr[0];
	pthread_mutex_unlock(&redisdata->g_mutex_data);

	redisdata->run_flag = run_flag;
	res = pthread_create(&redisdata->tid, NULL, major_thread, (void *)redisdata);	
	if(res != 0)
	{
		va_cout("create thread failed\n");
		ret = E_CREATETHREAD;
		goto exit;
	}

	v_thread_id.push_back(redisdata->tid);
	va_cout("initglobalData ok, will leave!");
	va_cout("addr return is %p", redisdata);
	*pctx = (uint64_t)redisdata;

exit:
	if(ret != E_SUCCESS)
	{
		if(redisdata != NULL)
		{
			if(redisdata->g_context != NULL)
			{
				redisFree(redisdata->g_context);
				redisdata->g_context = NULL;
			}
			if(redisdata->addr[0] != NULL)
			{
				delete redisdata->addr[0];
				redisdata->addr[0] = NULL;
			}
			if(redisdata->addr[1] != NULL)
			{
				delete redisdata->addr[1];
				redisdata->addr[1] = NULL;
			}
			delete redisdata;
		}
	}
	return ret;
}

void uninit_redis_operation(uint64_t arg)
{
	REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)arg;
	del_semvalue(redisdata->semidarr[0]);
	del_semvalue(redisdata->semidarr[1]);
//	pthread_cancel(redisdata->tid);
//	pthread_join(redisdata->tid, NULL);
	if(redisdata->g_context != NULL)
	{
		redisFree(redisdata->g_context);
		redisdata->g_context = NULL;
	}
	//redisFree(redisdata->pubsub);
	cout <<"major thread quit" <<endl;
	/*
	{
		pthread_mutex_destroy(&redisdata->g_mutex_data);
		delete redisdata->addr[0];
		delete redisdata->addr[1];
		delete redisdata;
	}*/
	return;
}


/*int main()
{
	pthread_t threadid, threadid2;
	pthread_t thidarr[10];
	threadid = threadid2 = 0;
	int res;
	int ret;
	
	uint64_t ctx;
	uint64_t logid = openLog("/etc/dspads_odin/dsp_amax.conf","locallog", true);

	ret = init_redis_operation(logid, ADX_AMAX,  SERVER_LOCAL, REDIS_PORT, SERVER_LOCAL, REDIS_PORT , &ctx);;
	if(ret == 0)
	{
		printf("parse success\n");
		uninit_redis_operation(ctx);
		//REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)ctx;
		//writeLog(logid, LOGINFO, "hello world");
		//sleep(50);
		//va_cout("semid1 is %d\n", redisdata->semidarr[0]);
		//va_cout("semid2 is %d\n", redisdata->semidarr[1]);

		//res = pthread_create(&threadid2, NULL, operate, (void *)ctx);
		//if(res != 0)
		//{
		//printf("create thread failed\n");
		//}
	}
	
	
	//pthread_join(threadid2, NULL);
	return 0;
}*/
