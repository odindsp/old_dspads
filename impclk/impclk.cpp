#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <hiredis/hiredis.h>
#include "impclk.h"
#include "../common/json_util.h"

using namespace std;

extern pthread_mutex_t g_mutex_id, g_mutex_ur, g_mutex_group;
extern vector<string> cmds_id;
extern vector<GROUP_FREQUENCY_INFO> v_group_frequency;
extern vector<USER_INFO> v_user_frequency;

extern uint64_t g_logid_local, g_logid;

extern "C"
{
#include <dlfcn.h>
	//  #include <ccl/ccl.h>
}

void init_group_frequency_info(GROUP_FREQUENCY_INFO &gr_fre_info)
{
	gr_fre_info.adx = gr_fre_info.type = gr_fre_info.interval = 0;
}

void writeUserRestriction_impclk(uint64_t logid_local, USER_FREQUENCY_COUNT &ur, string &command)
{
	char *text = NULL;
	json_t *root = NULL, *label_gr = NULL, 
		*label_cr = NULL, *array_gr = NULL, 
		*array_cr = NULL, *entry_gr = NULL, 
		*entry_cr = NULL;
	map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >::iterator user_fre;
	root = json_new_object();
	if (root == NULL)
		goto exit;

	jsonInsertString(root, "userid", ur.userid.c_str());
	label_gr = json_new_string("group_restriction");
	array_gr = json_new_array();

	user_fre  = ur.user_group_frequency_count.begin();
	while (user_fre != ur.user_group_frequency_count.end())
	{
		string groupid = user_fre->first;
		entry_gr = json_new_object();
		jsonInsertString(entry_gr, "groupid", groupid.c_str());

		map<string, USER_CREATIVE_FREQUENCY_COUNT>::iterator creative_fre = (user_fre->second).begin();
		label_cr = json_new_string("creative_restriction");
		array_cr = json_new_array();
		while (creative_fre != (user_fre->second).end())
		{
			string mapid = creative_fre->first;
			USER_CREATIVE_FREQUENCY_COUNT *cr = &creative_fre->second;
			entry_cr = json_new_object();
			jsonInsertString(entry_cr, "mapid", mapid.c_str());
			jsonInsertInt(entry_cr, "starttime", cr->starttime);
			jsonInsertInt(entry_cr, "imp", cr->imp);
			json_insert_child(array_cr, entry_cr);
			++creative_fre;
		}
		json_insert_child(label_cr, array_cr);
		json_insert_child(entry_gr, label_cr);
		json_insert_child(array_gr, entry_gr);
		++user_fre;
	}
	json_insert_child(label_gr, array_gr);
	json_insert_child(root, label_gr);
	json_tree_to_string(root, &text);

	if (root != NULL)
		json_free_value(&root);

	command = text;
	/*command = "SETEX dsp_user_frequency_" + ur.userid + " " + intToString(DAYSECOND - getCurrentTimeSecond()) + " " + string(text); 

	pthread_mutex_lock(&g_mutex_ur);
	cmds_ur.push_back(command);
	pthread_mutex_unlock(&g_mutex_ur);*/

exit:
	if (text != NULL)
		free(text);
}

int GroupRestrictionCount(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid_local, GROUP_FREQUENCY_INFO *gr_frequency_info)
{
	string value = "";
	char *text = NULL;
	json_t *root = NULL, *label = NULL;
	string set_command = "", enpire_command = "";
	int errcode = E_SUCCESS;

	int period_array[] = {15, 30, 60};
	char *dst_array[] = {"error", "dstimp", "dstclk"};
	char *sum_array[] = {"error", "sumimp", "sumclk"};
	char *finish_array[] = {"error", "s_imp_t", "s_clk_t"};
	char *total_array[] = {"error", "sumimp", "sumclk"};
	char *interval_array[] = {"error", "imp", "clk"};

	uint8_t period = gr_frequency_info->interval;
	const char *mtype = gr_frequency_info->mtype.c_str();
	struct timespec ts1, ts2;
	long lasttime = 0;
	getTime(&ts1);

	char keys_name[MIN_LENGTH] = "", cur_interval[MIN_LENGTH] = "", 
		sum_imp_num[MIN_LENGTH] = "", sum_clk_num[MIN_LENGTH] = "",
		dst_impclk[MIN_LENGTH] = "", finish_impclk_time[MIN_LENGTH] = "",
		sum_impclk_str[MIN_LENGTH] = "", dst_impclk_url[MIN_LENGTH] = "";

	struct timeval timeofday;
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	int url_type = 0;
	if (mtype[0] == 'm')
	{
	   url_type = PXENE_DELIVER_TYPE_IMP;
	}
	else if(mtype[0] == 'c')
	{
	   url_type = PXENE_DELIVER_TYPE_CLICK;
	}
	else  // wrong url 
	{
		writeLog(logid_local, LOGDEBUG, "wrong mtype:%c", mtype[0]);
		goto resource_free;	
	}

	/* Create all of the keys */
	sprintf(keys_name, "dsp_group_frequency_%02d_%02d", gr_frequency_info->adx, tm.tm_mday);

	if (period != RESTRICTION_PERIOD_DAY)
	{
		sprintf(cur_interval, "%02d%02d-%02d", tm.tm_hour,
			period_array[gr_frequency_info->interval] * (tm.tm_min / period_array[gr_frequency_info->interval]),
			period_array[gr_frequency_info->interval]);
		sprintf(finish_impclk_time, "%s_%02d", finish_array[gr_frequency_info->type], tm.tm_hour);
		sprintf(dst_impclk, "%s-%02d", dst_array[gr_frequency_info->type], tm.tm_hour);
		
		sprintf(dst_impclk_url, "%s-%02d", dst_array[url_type], tm.tm_hour);

		sprintf(sum_imp_num, "sumimp-%02d", tm.tm_hour);
		sprintf(sum_clk_num, "sumclk-%02d", tm.tm_hour);
		sprintf(sum_impclk_str, "%s-%02d", sum_array[url_type] , tm.tm_hour);
	}
	else
	{
		sprintf(dst_impclk, "%s", dst_array[gr_frequency_info->type]);
	}

	errcode = hgetRedisValue(ctx_r, logid_local, string(keys_name), gr_frequency_info->groupid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		writeLog(logid_local, LOGDEBUG, "hget fre context invalid");
		errcode = E_REDIS_CONNECT_R_INVALID;  // read context invalid
		goto resource_free;
	}

	if (value == "")
	{
		int imp = 1, clk = 0;												//为该项目创建初始粒度的imp or clk
		json_t *entry_gr = NULL, *entry_interval = NULL;

		root = json_new_object();
		if (root == NULL)
		{
			writeLog(logid_local, LOGDEBUG, "json_new_object is null");
			goto resource_free;
		}
		if (mtype[0] == 'c')
		{
		   imp = 0;
		   clk = 1;
		}

		jsonInsertInt(root, total_array[1], imp);
		jsonInsertInt(root, total_array[2], clk);

		if (period == RESTRICTION_PERIOD_DAY)
			jsonInsertInt(root, dst_impclk, gr_frequency_info->frequency[0]);//if the restriction was per-day, then choose the first members
		else
		{
			entry_interval = json_new_string(cur_interval);
			entry_gr = json_new_object();

			jsonInsertInt(root, sum_imp_num, imp);
			jsonInsertInt(root, sum_clk_num, clk);

			jsonInsertInt(entry_gr, interval_array[1], imp);
			jsonInsertInt(entry_gr, interval_array[2], clk);

			jsonInsertInt(root, dst_impclk, gr_frequency_info->frequency[tm.tm_hour]);
			
			json_insert_child(entry_interval, entry_gr);
			json_insert_child(root, entry_interval);
		}

		json_tree_to_string(root, &text);

		errcode = hsetRedisValue(ctx_w, logid_local, keys_name, gr_frequency_info->groupid, string(text));
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			goto resource_free;
		}
		errcode = expireRedisValue(ctx_w, logid_local, keys_name, DAYSECOND*20 - getCurrentTimeSecond());
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			goto resource_free;
		}
		goto resource_free;	
	}
	
	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
	    writeLog(logid_local, LOGDEBUG, "json_new_object is null");
		goto resource_free;	
	}

	//全天总数
	if (((label = json_find_first_label(root, total_array[url_type])) != NULL))
	{
		uint64_t sum_imp_clk = atoi(label->child->text);
		json_t *label_temp = NULL;

		char *new_sum_imp_clk = (char *)malloc(10 * sizeof(char));
		memset(new_sum_imp_clk, 0 , 10 * sizeof(char));
		sprintf(new_sum_imp_clk, "%d", sum_imp_clk + 1);

		free(label->child->text);
		label->child->text = new_sum_imp_clk;
	}
	else
	{
		jsonInsertInt(root, total_array[url_type], 1);
	}


	//Dst Number 每个小时的目标数，会根据上个时段的投放量进行滚动投放
	if ((label = json_find_first_label(root, dst_impclk)) == NULL)
	{
		json_t *label_temp = NULL;
		int64 dst_impclk_cur = 0;

		if (period == RESTRICTION_PERIOD_DAY)	//支持修改粒度
		{
			int i = tm.tm_hour;

			while (i >= 0)
			{
				int64 sum_impclk_num = 0;
				char sum_impclk_old[MIN_LENGTH] = "";

				sprintf(sum_impclk_old, "%s-%02d", sum_array[gr_frequency_info->type], i);

				if ((label_temp = json_find_first_label(root, sum_impclk_old)) != NULL)
					sum_impclk_num = atoi(label_temp->child->text);

				dst_impclk_cur += sum_impclk_num;

				i--;
			}

			dst_impclk_cur = gr_frequency_info->frequency[0] - dst_impclk_cur;

			if (gr_frequency_info->frequency[0] == 0)
				dst_impclk_cur = 0;
		}
		else
		{
			int i = tm.tm_hour - 1;

			while(i > 0)
			{
				int64 dst_impclk_num = 0, sum_impclk_num = 0;
				char dst_impclk_old[MIN_LENGTH] = "", sum_impclk_old[MIN_LENGTH] = "";

				sprintf(dst_impclk_old, "%s-%02d", dst_array[gr_frequency_info->type], i);
				sprintf(sum_impclk_old, "%s-%02d", sum_array[gr_frequency_info->type], i);

				if ((label_temp = json_find_first_label(root, dst_impclk_old)) != NULL)
					dst_impclk_num = atoi(label_temp->child->text);
				if ((label_temp = json_find_first_label(root, sum_impclk_old)) != NULL)
					sum_impclk_num = atoi(label_temp->child->text);

				if (dst_impclk_num)
				{
					dst_impclk_cur += dst_impclk_num - sum_impclk_num;	
					break;
				}

				i--;
			}

			dst_impclk_cur += gr_frequency_info->frequency[tm.tm_hour];
			
			if ((dst_impclk_cur <= 0) && (gr_frequency_info->frequency[tm.tm_hour] != 0))
				dst_impclk_cur = 1;
			
			if (gr_frequency_info->frequency[tm.tm_hour] == 0)
				dst_impclk_cur = 0;
		}
		jsonInsertInt(root, dst_impclk, dst_impclk_cur);//只记录type里面的展现或者点击的目前值
	}

	//Sum_imp_num and Sum_click_num 每小时的总数
	if (((label = json_find_first_label(root, sum_impclk_str)) != NULL))
	{
		uint64_t sum_imp = atoi(label->child->text);
		json_t *label_temp = NULL;

		char *new_sum_imp = (char *)malloc(10 * sizeof(char));
		memset(new_sum_imp, 0 , 10 * sizeof(char));
		sprintf(new_sum_imp, "%d", sum_imp + 1);

		free(label->child->text);
		label->child->text = new_sum_imp;

		++sum_imp;

		if (gr_frequency_info->type == url_type && ((label_temp = json_find_first_label(root, dst_impclk_url)) != NULL))
		{
			json_t *label_finish = NULL;
			uint64_t dst_cur_imp_num = 0;
			dst_cur_imp_num = atoi(label_temp->child->text);
			va_cout("%s,sum_impclk:%d,dst_cur_imp_num:%d", dst_impclk_url, sum_imp, dst_cur_imp_num);
			
			if ((dst_cur_imp_num != 0) && (dst_cur_imp_num <= sum_imp))
			{
				va_cout("finish_time:%s", finish_impclk_time);
				if ((label_finish = json_find_first_label(root, finish_impclk_time)) == NULL)
				{
					jsonInsertInt(root, finish_impclk_time, tm.tm_min);
					writeLog(logid_local, LOGDEBUG, "finish_time,%02d,%s,%s:%d", gr_frequency_info->adx, gr_frequency_info->groupid.c_str(), finish_impclk_time, tm.tm_min);
				}	
			}
		}
	}
	else
	{
		if (period != RESTRICTION_PERIOD_DAY)
		{
			jsonInsertInt(root, sum_impclk_str, 1);
		}
	}

	//修改某个interval中的imp or click
	if ((label = json_find_first_label(root, cur_interval)) != NULL)
	{
		json_t *temp_value = label->child;

		if (temp_value != NULL)
		{
			if (((label = json_find_first_label(temp_value, interval_array[url_type])) != NULL))
			{
				uint64_t num_imp = atoi(label->child->text);

				char *new_num_imp = (char *)malloc(10 * sizeof(char));
				memset(new_num_imp, 0 , 10 * sizeof(char));

				sprintf(new_num_imp, "%d", num_imp + 1);

				free(label->child->text);
				label->child->text = new_num_imp;
			}
		}
	}
	else//如果没有interval 那么就添加
	{
		int imp = 0, clk = 0;

		if (period != RESTRICTION_PERIOD_DAY)
		{
			json_t *entry_gr = NULL, *entry_interval = NULL;

			entry_interval = json_new_string(cur_interval);
			entry_gr = json_new_object();

			if(mtype[0] == 'm')
				imp = imp + 1;
			else
				clk = clk + 1;

			jsonInsertInt(entry_gr, "imp", imp);
			jsonInsertInt(entry_gr, "clk", clk);

			json_insert_child(entry_interval, entry_gr);
			json_insert_child(root, entry_interval);
		}
	}

exit:
	json_tree_to_string(root, &text);

	errcode = hsetRedisValue(ctx_w, logid_local, keys_name, gr_frequency_info->groupid, string(text));
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
	}

resource_free:
	if (root != NULL)
		json_free_value(&root);

	if (text != NULL)
		free(text);

	return errcode;
}

int check_group_frequency_capping(redisContext *ctx_r, redisContext *ctx_w, GROUP_FREQUENCY_INFO *gr_frequency_info)
{
	int errcode = E_SUCCESS;

	if (gr_frequency_info == NULL)
		goto exit;
	
	if(gr_frequency_info->type != 0)
	{
		errcode = GroupRestrictionCount(ctx_r, ctx_w, g_logid_local, gr_frequency_info);
	}

exit:
	return errcode;
}

void create_new_frequency(FREQUENCY_RESTRICTION_USER &fru, string groupid, string s_mapid, USER_FREQUENCY_COUNT &ur)
{
	USER_CREATIVE_FREQUENCY_COUNT cr_new;
	map<string, USER_CREATIVE_FREQUENCY_COUNT> creaive_temp;

	string mapid;
	if (fru.type == USER_FREQUENCY_CREATIVE_1 || fru.type == USER_FREQUENCY_CREATIVE_2)
		mapid = s_mapid;
	else
		mapid = groupid;

	cr_new.starttime = getStartTime(fru.period);
	cr_new.imp = 1;

	creaive_temp.insert(make_pair<string, USER_CREATIVE_FREQUENCY_COUNT>(mapid, cr_new));
	ur.user_group_frequency_count.insert(make_pair<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >(groupid, creaive_temp));
	return;
}

int UserRestrictionCount(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid_local, USER_INFO *ur_frequency_info)
{
	struct timespec ts1;
	int errcode = E_SUCCESS;
	USER_FREQUENCY_COUNT ur;
	USER_CREATIVE_FREQUENCY_COUNT *cr = NULL;
	string command;
	int intervaltime = 0;
	int fre_capping = 0;

	// 用户频次
	getTime(&ts1);
	errcode = readUserRestriction(ctx_r, logid_local, string(ur_frequency_info->deviceid), ur);
	record_cost(logid_local, true, ur_frequency_info->deviceid.c_str(), "readUserRestriction after", ts1);

	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		writeLog(logid_local, LOGDEBUG, "readUserRestriction  context invalid");
		errcode = E_REDIS_CONNECT_R_INVALID;  // read context invalid
		goto exit;
	}
	{
		map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >::iterator group_user_fre = ur.user_group_frequency_count.find(ur_frequency_info->groupid);

		if (group_user_fre == ur.user_group_frequency_count.end())
		{
			create_new_frequency(ur_frequency_info->fru, ur_frequency_info->groupid, ur_frequency_info->mapid, ur);
			goto writeur;
		}
		else
		{
			map<string, USER_CREATIVE_FREQUENCY_COUNT> &creaive_temp = group_user_fre->second;
			cr = NULL;
			string str_mapid;
			if ((ur_frequency_info->fru.type == USER_FREQUENCY_CREATIVE_2) || (ur_frequency_info->fru.type == USER_FREQUENCY_CREATIVE_1))
			{
				str_mapid = ur_frequency_info->mapid;
				if(ur_frequency_info->fru.type == USER_FREQUENCY_CREATIVE_2)
				{
					map<string, uint32_t>::iterator m_capping_it = ur_frequency_info->fru.capping.find(string(str_mapid));
					if(m_capping_it == ur_frequency_info->fru.capping.end())
					{
						writeLog(logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,mapid:%s,frequencycapping not find", ur_frequency_info->deviceid.c_str(), ur_frequency_info->groupid.c_str(),ur_frequency_info->mapid.c_str());
						goto exit;
					}
					else
					{
						fre_capping = m_capping_it->second;
					}
				}
				else
				{
					fre_capping = ur_frequency_info->fru.capping.begin()->second;
				}
			}
			else
			{
				str_mapid = ur_frequency_info->groupid;
				fre_capping = ur_frequency_info->fru.capping.begin()->second;
			}

			map<string, USER_CREATIVE_FREQUENCY_COUNT>::iterator p_creative = creaive_temp.find(str_mapid);
			if (p_creative != creaive_temp.end())
			{
				cr =  &(p_creative->second);
				if (cr != NULL)
				{
					if (cr->imp >= fre_capping)
					{
						if(getCurrentTimeSecond() - cr->starttime < PERIOD_TIME(ur_frequency_info->fru.period))
						{
							writeLog(logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,mapid:%s,has been shown enough time!",ur_frequency_info->deviceid.c_str(),ur_frequency_info->groupid.c_str(),ur_frequency_info->mapid.c_str());
							goto exit;
						}

						cr->starttime = getStartTime(ur_frequency_info->fru.period);
						cr->imp = 1;
						goto writeur;
					}

					intervaltime = getCurrentTimeSecond() - cr->starttime - PERIOD_TIME(ur_frequency_info->fru.period);
					if (intervaltime >= 0)
					{
						cr->starttime = getStartTime(ur_frequency_info->fru.period);
						cr->imp = 1;
					}
					else
					{
						++cr->imp;
					}
				}
				else
				{
					writeLog(logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,mapid:%s,cr is NULL!",ur_frequency_info->deviceid.c_str(),ur_frequency_info->groupid.c_str(),ur_frequency_info->mapid.c_str());
					goto exit;
				}
			}
			else
			{
				USER_CREATIVE_FREQUENCY_COUNT creative_r;
				creative_r.starttime = getStartTime(ur_frequency_info->fru.period);
				creative_r.imp = 1;
				creaive_temp.insert(make_pair<string, USER_CREATIVE_FREQUENCY_COUNT>(str_mapid, creative_r));
				goto writeur;
			}
		}
writeur:
		writeUserRestriction_impclk(g_logid_local, ur, command);
		if (command != "")
		{
			errcode = setexRedisValue(ctx_w ,logid_local, "dsp_user_frequency_" + ur.userid,
				DAYSECOND - getCurrentTimeSecond(), command);
			if (errcode == E_REDIS_CONNECT_INVALID)
			{
				errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			}
		}
	}
exit:
	return errcode;
}


bool check_frequency_capping(uint64_t gctx, int index, REDIS_INFO *cache, uint8_t adx, char *mapid, char *deviceid, char *mtype, double price)
{
	USER_CREATIVE_FREQUENCY_COUNT *cr = NULL;
	bool ret = false;
	GROUP_FREQUENCY_INFO gr_frequency_info;
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)gctx;

	if ((gctx == 0) || (cache == NULL) || (mapid == NULL) || (deviceid == NULL) || (mtype == NULL))
	{
		ret = false;
		writeLog(g_logid_local, LOGERROR, "gctx|cache|mapid|deviceid|mtype is invalid");
		goto exit;
	}
	{
		map<string, IMPCLKCREATIVEOBJECT>::iterator m_creative_it = cache->creativeinfo.find(string(mapid));

		if (m_creative_it == cache->creativeinfo.end())
		{
			ret = false;
			writeLog(g_logid_local, LOGERROR, "deviceid:%s,can't find mapid:%s in creativeinfo",deviceid,mapid);
			goto exit;
		}

		IMPCLKCREATIVEOBJECT &creativeinfo = m_creative_it->second;
		/*if (price)
		{
		ADX_PRICE_INFO temp_price;
		temp_price.adx = adx;
		temp_price.groupid = creativeinfo.groupid;
		temp_price.mapid = mapid;
		temp_price.price = price;

		pthread_mutex_lock(&pctx->mutex_price_info);
		pctx->v_price_info.push_back(temp_price);
		pthread_mutex_unlock(&pctx->mutex_price_info);
		}*/

		map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK>::iterator m_frequency_it = cache->fc.find(creativeinfo.groupid);

		if (m_frequency_it == cache->fc.end())
		{
			ret = false;
			writeLog(g_logid_local, LOGERROR, "deviceid:%s,can't find groupid:%s",deviceid,creativeinfo.groupid.c_str());
			goto exit;
		}

		FREQUENCYCAPPINGRESTRICTIONIMPCLK &frequencycapping = m_frequency_it->second;

		//将period/mtype/groupid插入vecter
		map<uint8_t, FREQUENCY_RESTRICTION_GROUP>::iterator m_frequency_group_it = frequencycapping.frg.find(adx);

		if (m_frequency_group_it == frequencycapping.frg.end())
		{
			writeLog(g_logid_local, LOGERROR, "deviceid:%s,groupid:%s,mapid:%s,adx:%d,not find adx frequency",deviceid,creativeinfo.groupid.c_str(),mapid,adx);
			ret = false;
			goto exit;
		}

		// 插入项目频次
		FREQUENCY_RESTRICTION_GROUP &group_frequency = m_frequency_group_it->second;

		init_group_frequency_info(gr_frequency_info);
		gr_frequency_info.interval = group_frequency.period;
		gr_frequency_info.groupid = creativeinfo.groupid;
		gr_frequency_info.mtype = string(mtype);
		gr_frequency_info.type = group_frequency.type;
		gr_frequency_info.frequency = group_frequency.frequency;
		gr_frequency_info.adx = adx;
		pthread_mutex_lock(&g_mutex_group);
		v_group_frequency.push_back(gr_frequency_info);
		pthread_mutex_unlock(&g_mutex_group);

		//从这里区分m和c
		if(mtype[0] == 'm')
		{
			if (frequencycapping.fru.type == USER_FREQUENCY_INFINITY)   // 不限制
			{
				ret = true;
				writeLog(g_logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,doesn't limit user frequence", deviceid, creativeinfo.groupid.c_str());
				goto exit;
			}

			USER_INFO user_info;
			user_info.deviceid = deviceid;
			user_info.groupid = creativeinfo.groupid;
			user_info.mapid = mapid;
			user_info.fru = frequencycapping.fru;
			pthread_mutex_lock(&g_mutex_ur);
			v_user_frequency.push_back(user_info);
			pthread_mutex_unlock(&g_mutex_ur);
		}
	}
	ret = true;	
exit:
	return ret;
}

int parseRequest(IN FCGX_Request request, IN  uint64_t gctx, IN int index, IN DATATRANSFER *datatransfer, IN DATATRANSFER *datatransfer_s, IN char *requestaddr, IN char *requestparam)
{
	struct timespec ts1, ts2;
	string str_flag = "";
	sem_t sem_id;
	char *adxtype = NULL, *mtype = NULL, *mapid = NULL, *id = NULL, *deviceid = NULL, *deviceidtype = NULL, 
		 *destination_url = NULL, *send_data = NULL, *strprice = NULL, *appsid = NULL, *advertiser = NULL;
	char slog[MAX_LENGTH] = "";
	char deappid[MAX_LENGTH] = "";
	char demodel[MAX_LENGTH] = "";

	char *appid, *nw, *os, *tp, *reqip, *gp, *mb, *op, *md;
	 appid = nw = os = tp = reqip = gp = mb = op = md = NULL;
	
	char* datasrc = NULL;
	
	 int i_nw, i_os, i_tp, i_op, i_mb, i_gp;
	 i_nw = i_os = i_tp = i_op = 0;
	 i_mb = 255;
	 i_gp = 1156000000;

	double price = 0;
	uint64_t redis_info_addr = 0;
	long lasttime = 0, request_len = 0;
	int flag = 0, errcode = E_SUCCESS;
	uint8_t iadx = 0;
	bool idflag = false;
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)gctx;
	GETCONTEXTINFO *ctxinfo = NULL;

	string decode_curl;

	getTime(&ts1);

	if ((gctx == 0) || (datatransfer == NULL) || (requestaddr == NULL) || (requestparam == NULL) || datatransfer_s == NULL)
	{
		errcode = E_INVALID_PARAM;
		writeLog(g_logid_local, LOGERROR, "request params invalid,err:0x%x", errcode);
		goto exit;
	}

	ctxinfo = &(pctx->redis_fds[index]);
	request_len = strlen(requestparam);

	/* 1.Required Parameters */
	mtype = getParamvalue(requestparam, "mtype");
	if ((mtype == NULL) || (strlen(mtype) > PXENE_MTYPE_LEN))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "mtype was null or invalid,err:0x%x", errcode);
		goto exit;
	}

	if (mtype[0] == 'a' || mtype[0] == 'v')
	{
	    errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "mtype:%c value is invalid", mtype[0]);
		goto exit;
	}

	mapid = getParamvalue(requestparam, "mapid");
	id = getParamvalue(requestparam, "bid");
	datasrc = getParamvalue(requestparam, "datasrc");
	//目前仅 dsp 和 pap 平台，NULL 的都是 dsp 平台
	if (datasrc == NULL)
	{
		datasrc = (char *)calloc(4, sizeof(char));
		strncpy(datasrc, "dsp", 4);
	}
	
	/* 2. App monitor */
	appsid = getParamvalue(requestparam, "appsid");
	if ((appsid != NULL) && (strlen(appsid) < PXENE_APPSID_LEN))
	{
		advertiser = getParamvalue(requestparam, "adv");
		if ((advertiser != NULL) && (strlen(advertiser) <= PXENE_ADV_LEN))
		{
			writeLog(g_logid_local, LOGDEBUG, "App monitor->appid:%s,advertiser:%s", appsid, advertiser);
			goto redirection;
		}
		else
		{
			errcode = E_INVALID_PARAM_REQUEST;
			writeLog(g_logid_local, LOGERROR, "app moniter request invalid");
			goto exit;
		}
	}

redirection:

	/* Device Info */
	appid = getParamvalue(requestparam, "appid");
	nw = getParamvalue(requestparam, "nw");
	os = getParamvalue(requestparam, "os");
	tp = getParamvalue(requestparam, "tp");
	reqip = getParamvalue(requestparam, "reqip");
	gp = getParamvalue(requestparam, "gp");
	mb = getParamvalue(requestparam, "mb");
	op = getParamvalue(requestparam, "op");
	md = getParamvalue(requestparam, "md");
	if(nw != NULL)
	{
		i_nw = atoi(nw);
	}
	if(os != NULL)
	{
		i_os = atoi(os);
	}
	if(tp != NULL)
	{
		i_tp = atoi(tp);
	}
	if(gp != NULL)
	{
		i_gp = atoi(gp);
	}
	if(mb != NULL)
	{
		i_mb = atoi(mb);
	}
	if(op != NULL)
	{
		i_op = atoi(op);
	}
	if (mtype[0] == 'c')
	{
		destination_url = getParamvalue(requestparam, "curl");
		if (destination_url != NULL && strlen(destination_url) > 0)
		{
			static pthread_mutex_t dest_mutex = PTHREAD_MUTEX_INITIALIZER;
			int decode_len = urldecode(destination_url, strlen(destination_url));

			decode_curl = destination_url;
			if(gp != NULL)
			{
				replaceMacro(decode_curl, "#GP#", gp);
				if(op != NULL)
				{
					replaceMacro(decode_curl, "#OP#", op);
				}
				if(mb != NULL)
				{
					replaceMacro(decode_curl, "#MB#", mb);
				}
				if(nw != NULL)
				{
					replaceMacro(decode_curl, "#NW#", nw);
				}
				if(tp != NULL)
				{
					replaceMacro(decode_curl, "#TP#", tp);
				}
			}
			if (id != NULL)
			{
				string str_id = id;
				if (str_id.find("%") == string::npos)
				{
					if (decode_curl.find("admaster") != string::npos)
					{
						if(decode_curl.find(",h") != string::npos)
						{
							decode_curl.insert(decode_curl.find(",h")+ 2, id);
						}
					}		
					replaceMacro(decode_curl, "#BID#", id);
				}
			}
			if (mapid != NULL)
			  replaceMacro(decode_curl, "#MAPID#", mapid);

			string destinationurl = "Location: " + string(decode_curl) + "\n\n";

			pthread_mutex_lock(&dest_mutex);
			FCGX_PutStr(destinationurl.data(), destinationurl.size(), request.out);
			pthread_mutex_unlock(&dest_mutex);
		}
	}

	if (((mapid == NULL) || (!check_string_type(mapid, PXENE_MAPID_LEN, true, false, INT_RADIX_16))))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "mapid was null or invalid,err:0x%x", errcode);
		goto exit;
	}

	if (appsid != NULL)
	{
		if (mtype[0] == 'c')
		{
			writeLog(g_logid, LOGDEBUG, "appsid:%s,adv:%s,mapid:%s,mtype:%c,curl:%s", appsid, advertiser, mapid, mtype[0], decode_curl.c_str());
		}
		else
		{
		   writeLog(g_logid, LOGDEBUG, "appsid:%s,adv:%s,mapid:%s,mtype:%c", appsid, advertiser, mapid, mtype[0]);
		}
		goto exit;
	}


	/* 4. ADX 0~255 */
	adxtype = getParamvalue(requestparam, "adx");
	if ((adxtype == NULL) || (strlen(adxtype) > PXENE_ADX_LEN) || (iadx > 255))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "adx was null or invalid");
		goto exit;
	}
	iadx = atoi(adxtype);

	deviceid = getParamvalue(requestparam, "deviceid");
	deviceidtype = getParamvalue(requestparam, "deviceidtype");
	if (((id == NULL) || (strlen(id) > PXENE_BID_LEN)) ||
		((deviceid == NULL) || (strlen(deviceid) > PXENE_DEVICEID_LEN)) ||
		((deviceidtype == NULL) || (strlen(deviceidtype) > PXENE_DEVICEIDTYPE_LEN)))
	{
		errcode = E_INVALID_PARAM_REQUEST;
		writeLog(g_logid_local, LOGERROR, "id/deviceid/deviceidtype was null or invalid");
		goto exit;
	}
	
	/* 6. Price */
	strprice = getParamvalue(requestparam, "price");
	if (strprice != NULL)
	{
		if (iadx == 0 || iadx > pctx->adx_path.size())   // 还未加载该adx的动态库
		{
			errcode = E_INVALID_PARAM;
			goto exit;
		}
		errcode = Decode(pctx->adx_path[iadx - 1], strprice, price);
		if (errcode != E_SUCCESS)
			writeLog(g_logid_local, LOGERROR, "Decode error,bid:%s,errcode:0x%x,encode_price:%s", id, errcode, strprice);
		else
			writeLog(g_logid_local, LOGDEBUG, "bid:%s,strprice:%s,price:%f",id,strprice, price);
	}

filter_request:

	if (ctxinfo->redis_id_slave == NULL)
	{
	    ctxinfo->redis_id_slave = redisConnect(pctx->slave_filter_id_ip.c_str(), pctx->slave_filter_id_port);
		va_cout("ctx->redis_id_slave is null");
		if (ctxinfo->redis_id_slave != NULL)
		{
			if (ctxinfo->redis_id_slave->err)
			{
				writeLog(g_logid_local, LOGDEBUG, "get id flag reconnect err:%s",ctxinfo->redis_id_slave->errstr);
				redisFree(ctxinfo->redis_id_slave);
				ctxinfo->redis_id_slave = NULL;
			}
		}
		else
		{
			writeLog(g_logid_local, LOGDEBUG, "get id flag reconnect NULL");
		}
	}

	if (ctxinfo->redis_id_slave != NULL)
	{
		errcode = getRedisValue(ctxinfo->redis_id_slave, g_logid_local, "dsp_id_flag_" + string(id), str_flag);
		record_cost(g_logid_local, true, id, "dsp_id_flag", ts1);

		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			va_cout("getRedisValue is E_REDIS_CONNECT_INVALID");
			redisFree(ctxinfo->redis_id_slave);
			ctxinfo->redis_id_slave = NULL;
		}
		if (str_flag != "")
		{
			flag = atoi(str_flag.c_str());

			if (!(flag & DSP_FLAG_SHOW) || !(flag & DSP_FLAG_CLICK))
			{
				char flagid[MIN_LENGTH] = "";
				string command = "";

				if (mtype[0] == 'm' && !(flag & DSP_FLAG_SHOW))
				{
					sprintf(flagid, "%d", flag | DSP_FLAG_SHOW);
				}
				else if (mtype[0] == 'c' && !(flag & DSP_FLAG_CLICK))
				{
					sprintf(flagid, "%d", flag | DSP_FLAG_CLICK);
				}
	
				//if some id have a impression or click, then the same request will set flagid to ""
				if(strlen(flagid) != 0)
				{
					command = "SETEX dsp_id_flag_" + string(id) + " " + intToString(REQUESTIDLIFE) + " " + string(flagid);

					pthread_mutex_lock(&g_mutex_id);
					cmds_id.push_back(command);
					pthread_mutex_unlock(&g_mutex_id);

					idflag = true;
					getTime(&ts2);
					lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
					writeLog(g_logid_local, LOGDEBUG, "id:%s,id_flag lasttime:%d", id, lasttime);
				}
				else
				{
					errcode = E_REPEATED_REQUEST;
					writeLog(g_logid_local, LOGERROR, "duplication request:%s,index=%d,adx=%d,mtype=%c", id, index, iadx, mtype[0]);
					goto exit;
				}
			}
			else
			{
				errcode = E_REPEATED_REQUEST;
				writeLog(g_logid_local, LOGERROR, "duplication request:%s,index=%d,adx=%d,mtype=%c", id, index, iadx, mtype[0]);
				goto exit;
			}
		}
		else
		{
			writeLog(g_logid_local, LOGERROR, "%s does not exist,index=%d,adx=%d,mtype=%c", id, index, iadx, mtype[0]);
			goto exit;
		}
	}


	//frequency capping,including user and groupid	
	get_semaphore(pctx->cache, &redis_info_addr, &sem_id);

	if(semaphore_release(sem_id)) //semaphore add 1
	{
		REDIS_INFO *redis_cashe = (REDIS_INFO *)redis_info_addr;
		writeLog(g_logid_local, LOGINFO, "redis cashe success");

		if (check_frequency_capping(gctx, index, redis_cashe, iadx, mapid, deviceid, mtype, price))
			writeLog(g_logid_local, LOGINFO, "bid:%s,frequencycapping add", id);

		if (!semaphore_wait(sem_id))
			writeLog(g_logid_local, LOGERROR, "bid:%s,semaphore_wait failed", id);
		
		record_cost(g_logid_local, true, (id != NULL ? id:"NULL"), "semaphore_wait after", ts1);
	}
	
feedback:
	if (idflag)
	{
		send_data = (char *)malloc(sizeof(char) * request_len);
		if ((send_data == NULL))
		{
			errcode = E_MALLOC;
			goto exit;
		}

		if (appid != NULL)
		{
		  strncpy(deappid, appid, MAX_LENGTH - 1);
		  deappid[MAX_LENGTH - 1] = '\0';
		  urldecode(deappid, strlen(deappid));
		}

		if (md != NULL)
		{
		  strncpy(demodel, md, MAX_LENGTH - 1);
		  demodel[MAX_LENGTH - 1] = '\0';
		  urldecode(demodel, strlen(demodel));
		}

		sprintf(send_data, "%s|%s|%s|%s|%c|%d|%s|%s|%d|%d|%d|%d|%d|%d|%s|%s", id, deviceid, deviceidtype, mapid,
				mtype[0], iadx, requestaddr, NULLSTR(appid), i_nw, i_os, i_tp, i_gp, i_mb, i_op, NULLSTR(md), datasrc);

		sendLog(datatransfer, string(send_data, strlen(send_data)));
		
		if (strprice != NULL)
		{
			sprintf(send_data, "%s|%s|%s|%s|%f|%d|%s|%s|%d|%d|%d|%d|%d|%d|%s|%s", id, deviceid, deviceidtype, mapid, price,
					iadx, requestaddr, NULLSTR(appid), i_nw, i_os, i_tp, i_gp, i_mb, i_op, NULLSTR(md), datasrc);
			sprintf(slog, "ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,mtype:%c,appid:%s,network:%d,os:%d,devicetype:%d,reqip:%s,reqloc:%d,brand:%d,carrier:%d,model:%s,price:%f,datasrc:%s",
					requestaddr, iadx, id, mapid, deviceid, deviceidtype, mtype[0], deappid, i_nw, i_os, i_tp, NULLSTR(reqip), i_gp, i_mb, i_op, demodel, price, datasrc);

			sendLog(datatransfer_s, string(send_data, strlen(send_data)));

		}
		else
			sprintf(slog, "ip:%s,adx:%d,bid:%s,mapid:%s,deviceid:%s,deviceidtype:%s,mtype:%c,appid:%s,network:%d,os:%d,devicetype:%d,reqip:%s,reqloc:%d,brand:%d,carrier:%d,model:%s, datasrc:%s",
					requestaddr, iadx, id, mapid, deviceid, deviceidtype, mtype[0], deappid, i_nw, i_os, i_tp, NULLSTR(reqip), i_gp, i_mb, i_op, demodel, datasrc);

		writeLog(g_logid, LOGDEBUG, string(slog));
	}

exit:
	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "%s,spent time:%ld,err:0x%x", (id != NULL ? id:"NULL"), lasttime, errcode);

	if (adxtype != NULL)
		free(adxtype);

	if (mtype != NULL)
		free(mtype);

	if (mapid != NULL)
		free(mapid);
	
	if (datasrc != NULL)
        free(datasrc);
	
	if (strprice != NULL)
		free(strprice);

	if (id != NULL)
		free(id);

	if (deviceid != NULL)
		free(deviceid);

	if (deviceidtype != NULL)
		free(deviceidtype);

	if (destination_url != NULL)
		free(destination_url);

	if (send_data != NULL)
		free(send_data);

	if (advertiser != NULL)
		free(advertiser);

	if (appsid != NULL)
		free(appsid);

	if (appid != NULL)
		free(appid);

	if (nw != NULL)
		free(nw);

	if (os != NULL)
		free(os);

	if (tp != NULL)
		free(tp);

	if (reqip != NULL)
		free(reqip);

	if (gp != NULL)
		free(gp);

	if (mb != NULL)
		free(mb);

	if (op != NULL)
		free(op);

	if (md != NULL)
		free(md);

	return errcode;
}
