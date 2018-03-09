#include <syslog.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>
#include <algorithm>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "util.h"
#include "setlog.h"
#include "type.h"
#include "tcp.h"
#include "json_util.h"
#include "errorcode.h"
#include <sys/time.h>
#include <sstream>  // 20170713 : add for new tracker

bool check_string_type(string data, int len, bool bconj, bool bcolon, uint8_t radix)
{
	if (data.length() != len)
		return false;

	for (int i = 0; i < data.length(); i++)
	{
		char c = data[i];

		if (radix != INT_IGNORE)
		{
			switch (radix)
			{
			case INT_RADIX_10:
				if ((c >= '0') && (c <= '9'))
					continue;
				break;

			case INT_RADIX_16_LOWER:
				if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')))
					continue;
				break;

			case INT_RADIX_16_UPPER:
				if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')))
					continue;
				break;

			case INT_RADIX_16:
				if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))
					continue;
				break;

			case INT_RADIX_2:
				if ((c >= '0') && (c <= '1'))
					continue;
				break;

			case INT_RADIX_8:
				if ((c >= '0') && (c <= '7'))
					continue;
				break;

			default:
				break;
			}
		}

		if (bconj)
		{
			if (c == '-')
				continue;
		}

		if (bcolon)
		{
			if (c == ':')
				continue;
		}

		return false;
	}

	return true;
}

void trim_left_right(INOUT string &str)
{
	if(!str.empty())
	{
		int i;
		int len = str.size();
		for(i = 0;i < len;++i)
		{
			if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
				break;
		}
		str.erase(0,i);
		len = str.size();
		for(i = len -1;i > 0;--i)
		{
			if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
				break;
		}
		str.erase(i+1,len);
	}
}

void trimstring(INOUT string &str)
{
	if(!str.empty())
	{
		int i;
		int len = str.size();
		for(i = 0;i < len;++i)
		{
			if ((str[i] != ' ') && (str[i] != '\t'))
				break;
		}
		str.erase(0,i);
		len = str.size();
		for(i = len -1;i > 0;--i)
		{
			if ((str[i] != ' ') && (str[i] != '\t'))
				break;
		}
		str.erase(i+1,len);
	}
}

bool init_category_table_t(IN const string file_path, IN CALLBACK_INSERT_PAIR cb_insert_pair, IN void *data, IN bool left_is_key)
{
	bool parsesuccess = false;
	ifstream file_in;
	if(file_path.length() == 0)
	{
		va_cout("file path is empty");
		goto exit;
	}

	file_in.open(file_path.c_str(), ios_base::in);
	if(file_in.is_open())
	{
		string str = "";
		while(getline(file_in, str))
		{
			string val, key;

			trim_left_right(str);

			if(str[0] != '#')
			{
				int pos_equal = str.find('=');
				if(pos_equal == string::npos)
					continue;

				key = str.substr(0, pos_equal);
				val = str.substr(pos_equal + 1);

				if (!left_is_key)
					key.swap(val);

				trim_left_right(val);
				trim_left_right(key);

				if(val.length() == 0)
					continue;
				if(key.length() == 0)
					continue;

				do 
				{
					string s_sub = key;

					int pos_comma = key.find(",");
					if (pos_comma != string::npos)
					{
						s_sub = key.substr(0, pos_comma);
						key = key.substr(pos_comma + 1);
					}
					else
					{
						key = "";
					}

					trim_left_right(s_sub);

					int pos_wave = s_sub.find('~');

					string s_start = (pos_wave != string::npos) ? s_sub.substr(0, pos_wave) : s_sub;
					string s_end = (pos_wave != string::npos) ? s_sub.substr(pos_wave + 1) : s_sub;

					trim_left_right(s_start);
					trim_left_right(s_end);

					cb_insert_pair(data, s_start, s_end, val);

				} while (key.length() != 0);
			}
		}

		parsesuccess = true;

		file_in.close();
	}
	else
	{
		va_cout("open config file error: %s", strerror(errno));
	}

exit:

	return parsesuccess;
}

void callback_insert_pair_make(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, uint16_t> &table = *((map<string, uint16_t> *)data);
	uint32_t com_cat;

	sscanf(val.c_str(), "%d", &com_cat);
	table[key_start] = com_cat;
	return;
}

template<typename T>
void jsonGetIntegerSet(json_t *label, set<T> &v)
{
	if (label == NULL || label->child == NULL || label->child->type != JSON_ARRAY)
		return;

	json_t *temp = label->child->child;

	while (temp != NULL)
	{
		if(temp->type == JSON_NUMBER)
			v.insert(atoi(temp->text));
		temp = temp->next;
	}
}

template<typename T>
string setIntToString(const set<T> &v, char separator)
{
	string str = "";
	if(v.size() == 0)
		return "NULL";

	string terminal(1, separator);

	typename set<T>::const_iterator iter = v.begin();
	str += uintToString(*iter);
	for(++iter; iter != v.end(); ++iter)
	{
		str +=  terminal;
		str += uintToString(*iter);
	}
	return str;
}

template<typename T>
string  vecIntToString(vector<T> &v)
{
	string str = "";
	if(v.size() == 0)
		return "NULL";

	string terminal(1, 1);

	typename vector<T>::iterator iter = v.begin();
	str += uintToString(*iter);
	for(++iter; iter != v.end(); ++iter)
	{
		str +=  terminal;
		str += uintToString(*iter);	
	}

	return str;
}

void init_DEVICE_TARGET_object(DEVICE_TARGET &dev_target)
{
	dev_target.flag = 0;
}

void init_APP_TARGET_object(APP_TARGET &app_target)
{
	app_target.id.flag = 0;
	app_target.cat.flag = 0;
}

void init_SCENE_TARGET_object(SCENE_TARGET &scene_target)
{
	scene_target.flag = 0;
	scene_target.length = DEFAULTLENGTH;
}

void init_GROUP_TARGET_object(GROUP_TARGET &target)
{
	//target.groupid = "";
	init_APP_TARGET_object(target.app_target);
	init_DEVICE_TARGET_object(target.device_target);
	init_SCENE_TARGET_object(target.scene_target);
}

void init_ADX_TARGET_object(ADX_TARGET &target)
{
	target.adx = ADX_UNKNOWN;
	init_APP_TARGET_object(target.app_target);
	init_DEVICE_TARGET_object(target.device_target);
}

void clear_DEVICE_TARGET_object(DEVICE_TARGET &dev_target)
{
	dev_target.flag = 0;
	dev_target.regioncode.clear();
	dev_target.connectiontype.clear();
	dev_target.os.clear();
	dev_target.carrier.clear();
	dev_target.devicetype.clear();
}

void clear_APP_TARGET_object(APP_TARGET &app_target)
{
	app_target.id.flag = 0;
	app_target.cat.flag = 0;
	app_target.cat.wlist.clear();
	app_target.cat.blist.clear();
	app_target.id.wlist.clear();
	app_target.id.blist.clear();
}

void clear_ADX_TARGET_object(ADX_TARGET &target)
{
	target.adx = ADX_UNKNOWN;
	clear_APP_TARGET_object(target.app_target);
	clear_DEVICE_TARGET_object(target.device_target);
}

void init_CREATIVE_object(CREATIVEOBJECT &creative)
{
	//creative.mapid = creative.groupid = creative.adid = creative.cbundle =creative.curl = creative.monitorcode = creative.sourceurl = creative.crid = creative.crid = "";
	creative.price = creative.type = creative.ftype = creative.ctype = creative.w = creative.h = creative.ftype = 0;
	creative.icon.ftype = creative.icon.w = creative.icon.h = 0;
	creative.rating = 255;
	creative.advcat = 0;
}

void init_IMG_OBJECT(IMAG_OBJECT &imgs)
{
	imgs.h = imgs.w = imgs.ftype = 0;
	imgs.sourceurl = "";
}
void init_WBLIST_object(WBLIST &wblist)
{
	//wblist.groupid = "";
	wblist.ratio = wblist.mprice = 0;
}

void init_FC_USER_object(FREQUENCY_RESTRICTION_USER &fru)
{
	fru.type = fru.period = 0;
}

void init_FC_GROUP_object(FREQUENCY_RESTRICTION_GROUP &frg)
{
	frg.type = frg.period = 0;
}

void init_FC_object(FREQUENCYCAPPINGRESTRICTION &fcr)
{
	//fcr.groupid = "";
	init_FC_USER_object(fcr.fru);
}

void init_FCIMPCLK_object(FREQUENCYCAPPINGRESTRICTIONIMPCLK &fcr)
{
	fcr.lastrefreshtime = getCurrentTimeSecond();
	init_FC_USER_object(fcr.fru);
}

void init_GROUPINFO_object(GROUPINFO &ginfo)
{
	ginfo.advcat = 0;
	ginfo.at = BID_RTB;
	ginfo.redirect = 0;
	ginfo.effectmonitor = 0;
	ginfo.is_secure = 0;
}

void init_ADXTEMPLATE(ADXTEMPLATE &adxinfo)
{
	adxinfo.adx  = adxinfo.ratio = 0;
	//adxinfo.aurl = adxinfo.nurl = "";
}

void init_ADMS(ADMS &adms)
{
	adms.os = adms.type = 0;
	//adms.adm = "";
}

void init_GROUP_SMART_BID(GROUP_SMART_BID_INFO &grsmartbidinfo)
{
	grsmartbidinfo.flag = grsmartbidinfo.maxratio = grsmartbidinfo.minratio = 0;
	grsmartbidinfo.interval = TIME_INTERVAL;
}

void init_LOG_CONTROL_INFO(LOG_CONTROL_INFO &logctrlinfo)
{
	logctrlinfo.ferr_level = FERR_LEVEL_TOP;
	logctrlinfo.is_print_time = 0;
}

long int getStartTime(uint8_t period)
{
    /*time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
	*/
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	switch(period)
	{
		case RESTRICTION_PERIOD_HOUR:
			return (tm.tm_hour * 60 * 60);
		case RESTRICTION_PERIOD_DAY:
			return 0;
	}
	return 0;
}

uint32_t getCurrentTimeSecond()
{
	/*time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	*/
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	return (tm.tm_hour * 60 * 60 + tm.tm_min * 60 + tm.tm_sec);
}

void getlocaltime(char *localtm)
{
	time_t now; //实例化time_t结构
	struct tm *timenow; //实例化tm结构指针
	time(&now);     //time函数读取现在的时ｿ国际标准时间非北京时ｿ，然后传值给now
	timenow = localtime(&now);
	char daytime[50];

	asctime_r(timenow, daytime);
	char *pch, *week, *mon, *day, *time, *year;
	pch = strtok(daytime, " ");
	int i = 0;
	while(pch != NULL)
	{
		switch(i)
		{
			case 0: week = pch; break;
			case 1: mon = pch; break;
			case 2: day = pch; break;
			case 3: time = pch; break;
			case 4: year = pch; break;
			default: break;
		}
		pch = strtok(NULL, " ");
		i++;
	}

	char realyear[10];
	strncpy(realyear, year, 4);
	sprintf(localtm, "%s, %s %s %s %s GMT\n", week, day, mon, realyear, time);
}

void getTime(struct timespec *ts)
{
	clock_gettime(CLOCK_REALTIME, ts);
}

char *getParamvalue(const char *inputparam, const char *paramname)
{
	char *p, *p2, *paramvalue;

	p2 = (char *)inputparam;
	char paramname_all[1024] = "";
	snprintf(paramname_all, 1024, "%s=", paramname);
	while (1)
	{
		if ((p = (char *)strstr(p2, paramname_all)) == NULL)
			return NULL;

		if (strcmp(p, p2) == 0 || p2[strlen(p2) - strlen(p) - 1] == '&')
			break;

		p2 = p + strlen(paramname_all);
	}

	p += strlen(paramname_all);
	p2 = strchr(p, '&');

	if (p2 == NULL)
		paramvalue = strdup(p);
	else if (p2 == p)
		return NULL;
	else
	{
		paramvalue = (char *)calloc(1, strlen(p) - strlen(p2) + 1);
		strncpy(paramvalue, p, strlen(p) - strlen(p2));
	}

	return paramvalue;
}

string intToString(int arg)
{
	char num[64];

	snprintf(num, 64, "%d", arg);

	return string(num);
}

string uintToString(uint64_t arg)
{
	char num[64];

	snprintf(num, 64, "%"PRIu64, arg);

	return string(num);
}

string doubleToString(double arg)
{
	char num[64] = "";

	snprintf(num, 64, "%lf", arg);

	return string(num);	
}

string hexToString(int arg)
{
	char num[64];

	snprintf(num, 64, "0x%x", arg);

	return string(num);
}

redisContext *connectRedisServer(char *ip, int port)
{
	redisContext *context = NULL;

reconnect:
	context = redisConnect(ip, port);
	if (context->err)
	{
		redisFree(context);
		syslog(LOG_INFO, "Connect redis server %s, port %d failed", ip, port);
		usleep(100000);
		goto reconnect;
	}
}

int getFrequencyCappingRestriction(redisContext *context, uint8_t adx, uint64_t logid_local, const char *groupid, FREQUENCYCAPPINGRESTRICTION &fcr)
{
	string value = "";
	int errcode = E_SUCCESS;
	json_t *root, *label, *temp;
	
	root = label = temp = NULL;
	errcode = getRedisValue(context, logid_local, "dsp_groupid_frequencycapping_" + string(groupid), value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}
	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;;
		writeLog(logid_local, LOGINFO, "dsp_groupid_frequencycapping_" + string(groupid) + " does not exist!");
		goto exit;
	}
	
	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
		writeLog(logid_local, LOGINFO, "getFrequencyCappingRestriction: root is NULL!");
		errcode = E_REDIS_GROUP_FC_INVALID;
		goto exit;
	}

	errcode = E_REDIS_GROUP_FC_INVALID;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "groupid", JSON_STRING))
		fcr.groupid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "group", JSON_ARRAY))
	{
		temp = label->child->child;
		while(temp)
		{
			uint8_t redisadx = 0;
			FREQUENCY_RESTRICTION_GROUP frg;
			init_FC_GROUP_object(frg);
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "adx", JSON_NUMBER))
				redisadx = atoi(label->child->text);

			if(adx == redisadx)
			{
				if(JSON_FIND_LABEL_AND_CHECK(temp, label, "type", JSON_NUMBER))
					frg.type = atoi(label->child->text);
				if(JSON_FIND_LABEL_AND_CHECK(temp, label, "period", JSON_NUMBER))
					frg.period = atoi(label->child->text);
				if(JSON_FIND_LABEL_AND_CHECK(temp, label, "frequency", JSON_ARRAY))
					jsonGetIntegerArray(label, frg.frequency);

				if(frg.frequency.size() > 0)
				{
					if(frg.frequency.size() != 1 && frg.frequency.size() != 24)
					{
						errcode = E_REDIS_GROUP_FC_INVALID;
						goto exit;
					}

					if(frg.period == RESTRICTION_PERIOD_DAY)
					{
						if(frg.frequency[0] == 0)
						{
							errcode = E_REDIS_GROUP_FC_INVALID;
							goto exit;
						}
					}
					else
					{
						if(frg.frequency.size() == 24)
						{
							struct tm tm = { 0 };
							time_t timep;

							time(&timep);
							localtime_r(&timep, &tm);
							if(frg.frequency[tm.tm_hour] == 0)
							{
								errcode = E_REDIS_GROUP_FC_INVALID;
								goto exit;
							}
						}
					}
				}
				
				fcr.frg.insert(make_pair(redisadx, frg));
				errcode = E_SUCCESS;
				goto out;
			}
			temp = temp->next;
		}
	}
out:
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "user", JSON_OBJECT))
	{
		temp = label->child;
		if(JSON_FIND_LABEL_AND_CHECK(temp, label, "type", JSON_NUMBER))
			fcr.fru.type = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(temp, label, "period", JSON_NUMBER))
			fcr.fru.period = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(temp, label, "capping", JSON_ARRAY))
		{
			json_t *tmp;
			tmp = label->child->child;
			int frequency;
			while(tmp)
			{
				if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "frequency", JSON_NUMBER))
				{
					frequency = atoi(label->child->text);
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "id", JSON_STRING))
					{
						fcr.fru.capping.insert(make_pair(string(label->child->text), frequency));
						//va_cout("groupid is %s and frequency: %d", label->child->text, frequency);
					}
				}
				tmp = tmp->next;
			}
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);

	return errcode;
}

int getFrequencyCappingRestrictionimpclk(redisContext *context, uint64_t logid_local, const char *groupid, FREQUENCYCAPPINGRESTRICTIONIMPCLK &fcr)
{
	string value = "";
	int errcode = E_SUCCESS;
	json_t *root, *label, *temp;

	root = label = temp = NULL;
	errcode = getRedisValue(context, logid_local, "dsp_groupid_frequencycapping_" + string(groupid), value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}
	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;;
		writeLog(logid_local, LOGINFO, "dsp_groupid_frequencycapping_" + string(groupid) + " does not exist!");
		goto exit;
	}

	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
		writeLog(logid_local, LOGINFO, "getFrequencyCappingRestriction: root is NULL!");
		errcode = E_REDIS_GROUP_FC_INVALID;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "groupid", JSON_STRING))
		fcr.groupid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "group", JSON_ARRAY))
	{
		temp = label->child->child;
		while(temp)
		{
			uint8_t redisadx = 0;
			FREQUENCY_RESTRICTION_GROUP frg;
			init_FC_GROUP_object(frg);
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "adx", JSON_NUMBER))
				redisadx = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "type", JSON_NUMBER))
				frg.type = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "period", JSON_NUMBER))
				frg.period = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "frequency", JSON_ARRAY))
				jsonGetIntegerArray(label, frg.frequency);

			fcr.frg.insert(make_pair(redisadx, frg));
			temp = temp->next;
		}
	}
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "user", JSON_OBJECT))
	{
		temp = label->child;
		if(JSON_FIND_LABEL_AND_CHECK(temp, label, "type", JSON_NUMBER))
			fcr.fru.type = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(temp, label, "period", JSON_NUMBER))
			fcr.fru.period = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(temp, label, "capping", JSON_ARRAY))
		{
			json_t *tmp;
			tmp = label->child->child;
			int frequency;
			while(tmp)
			{
				if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "frequency", JSON_NUMBER))
				{
					frequency = atoi(label->child->text);
					if(JSON_FIND_LABEL_AND_CHECK(tmp, label, "id", JSON_STRING))
					{
						fcr.fru.capping.insert(make_pair(string(label->child->text), frequency));
						//va_cout("groupid is %s and frequency: %d", label->child->text, frequency);
					}
				}
				tmp = tmp->next;
			}
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);

	return errcode;
}

int readUserRestriction(redisContext *context, uint64_t logid_local, string deviceid, USER_FREQUENCY_COUNT &user_restriction)
{
	string value = "";
	int errcode = E_SUCCESS;
	json_t *root, *label;

	root = label = NULL;
	user_restriction.userid = deviceid;

	errcode = getRedisValue(context, logid_local, "dsp_user_frequency_" + deviceid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		writeLog(logid_local, LOGINFO, "dsp_user_frequency_" + string(deviceid) + " does not exist!");
		errcode = E_REDIS_NO_CONTENT;
		goto exit;
	}

        json_parse_document(&root, value.c_str());
        if (root == NULL)
        {
		writeLog(logid_local, LOGINFO, "readUserRestriction: root is NULL!");
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
        }
		if(JSON_FIND_LABEL_AND_CHECK(root, label, "userid", JSON_STRING))
                user_restriction.userid = label->child->text;
		if(JSON_FIND_LABEL_AND_CHECK(root, label, "group_restriction", JSON_ARRAY))
        {
                json_t *t1 = label->child->child, *t2;

                while (t1 != NULL)
                {
					    string groupid;
						if(JSON_FIND_LABEL_AND_CHECK(t1, t2, "groupid", JSON_STRING))
                            groupid = t2->child->text;
						if(JSON_FIND_LABEL_AND_CHECK(t1, t2, "creative_restriction", JSON_ARRAY))
                        {
                                json_t *tt1 = t2->child->child, *tt2;
								map<string, USER_CREATIVE_FREQUENCY_COUNT> creative_map;
                                while (tt1 != NULL)
                                {
                                        USER_CREATIVE_FREQUENCY_COUNT creative_restriction;
										string mapid;
										if(JSON_FIND_LABEL_AND_CHECK(tt1, tt2, "mapid", JSON_STRING))
                                            mapid = tt2->child->text;
										if(JSON_FIND_LABEL_AND_CHECK(tt1, tt2, "starttime", JSON_NUMBER))
                                                creative_restriction.starttime = atoi(tt2->child->text);
										if(JSON_FIND_LABEL_AND_CHECK(tt1, tt2, "imp", JSON_NUMBER))
                                                creative_restriction.imp = atoi(tt2->child->text);
                                       
										creative_map.insert(make_pair<string, USER_CREATIVE_FREQUENCY_COUNT>(mapid, creative_restriction));

                                        tt1 = tt1->next;
                                }

								user_restriction.user_group_frequency_count.insert(make_pair<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >(groupid, creative_map));
                        }
                        
                        t1 = t1->next;
                }
        }
exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

int writeUserRestriction(redisContext *context, uint64_t logid_local, USER_FREQUENCY_COUNT &ur)
{
	int errcode = E_UNKNOWN;
	char *text = NULL;
        json_t *root, *label_gr, *label_cr, *array_gr, *array_cr, *entry_gr, *entry_cr;
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

	errcode = setexRedisValue(context ,logid_local, "dsp_user_frequency_" + ur.userid,
                        DAYSECOND - getCurrentTimeSecond(), text);
exit:
	if(text != NULL)
		free(text);
	return errcode;
}

void *threadSendLog(void *arg)
{
	int errorcode;
	DATATRANSFER *datatransfer = (DATATRANSFER *)arg;
	struct timespec ts1, ts2;
	long lasttime;
	int index = 0, size;

//	pthread_detach(pthread_self());

	getTime(&ts1);
	while ( *datatransfer->run_flag )
	{
		pthread_mutex_lock(&datatransfer->mutex_data);
		if (!datatransfer->data.empty())
		{
			string logdata;

			++index;
			size = datatransfer->data.size();
			logdata = datatransfer->data.front();
			datatransfer->data.pop();
			pthread_mutex_unlock(&datatransfer->mutex_data);

			errorcode = sendMessage(datatransfer->fd, logdata.data(), logdata.size());
			if (errorcode <= 0)
			{
				writeLog(datatransfer->logid, LOGDEBUG, "reconnect ip :%s :port :%d", datatransfer->server.c_str(), datatransfer->port );
				datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
				if(datatransfer->fd != -1)
				{
					sendMessage(datatransfer->fd, logdata.data(), logdata.size());
				}
				else
					writeLog(datatransfer->logid, LOGDEBUG, "reconnect ip :%s :port :%d failed", datatransfer->server.c_str(), datatransfer->port );
			}

			if (index % 1000 == 0)
			{
				getTime(&ts2);
				lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
				writeLog(datatransfer->logid, LOGDEBUG, "queue size: %d, total time: %ld us, average time: %ld us", size, lasttime, lasttime / 1000);
				index = 0;
				memcpy(&ts1, &ts2, sizeof(struct timespec));
			}
		}
		else
		{
			pthread_mutex_unlock(&datatransfer->mutex_data);
			usleep(100000);
		}
	}

	pthread_mutex_lock(&datatransfer->mutex_data);
	while (!datatransfer->data.empty())
	{
		string logdata;
		logdata = datatransfer->data.front();
		datatransfer->data.pop();
		if (datatransfer->fd != -1)
           sendMessage(datatransfer->fd, logdata.data(), logdata.size());
	}
	pthread_mutex_unlock(&datatransfer->mutex_data);

	cout<<"threadSendLog exit"<<endl;
	return NULL;
}

void *threadSendKafkaLog(void *arg)
{
	int errorcode;
	DATATRANSFER *datatransfer = (DATATRANSFER *)arg;
	struct timespec ts1, ts2;
	long lasttime;
	int index = 0, size;
	uint16_t topic_flag = 0;
	getTime(&ts1);
	KafkaServer &kafkaserver = KafkaServer::getInstance();
	while (*datatransfer->run_flag)
	{
		pthread_mutex_lock(&datatransfer->mutex_data);
		if (!datatransfer->data.empty())
		{
			string logdata;
			++index;
			size = datatransfer->data.size();
			logdata = datatransfer->data.front();
			datatransfer->data.pop();
			pthread_mutex_unlock(&datatransfer->mutex_data);
			errorcode = kafkaserver.sendMessage(&datatransfer->kafka_conf, logdata);
			//errorcode = sendMessageToKafka(logdata.data(), logdata.size());
			if (errorcode < 0)
			{
				writeLog(datatransfer->logid, LOGDEBUG, "reconnect broken_list :%s", datatransfer->kafka_conf.broker_list.c_str());
				datatransfer->fd = kafkaserver.connectServer(&datatransfer->kafka_conf);
				if (datatransfer->fd != -1)
				{
					//sendMessageToKafka(logdata.data(), logdata.size());
					kafkaserver.sendMessage(&datatransfer->kafka_conf, logdata);
				}
				else
				{
					writeLog(datatransfer->logid, LOGDEBUG, "reconnect broken_list :%s failed", datatransfer->kafka_conf.broker_list.c_str());
				}
			}
			if (index % 1000 == 0)
			{
				getTime(&ts2);
				lasttime = (ts2.tv_sec - ts1.tv_sec ) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
				writeLog(datatransfer->logid, LOGDEBUG, "queue size: %d, total time: %ld us, average time: %ld us", size, lasttime, lasttime / 1000);
				index = 0;
				memcpy(&ts1, &ts2, sizeof(struct timespec));
			}
		} // end of if (!datatransfer->data.empty())
		else
		{
			pthread_mutex_unlock(&datatransfer->mutex_data);
			usleep(100000);
		}
	} // end of while

	pthread_mutex_lock(&datatransfer->mutex_data);
	while (!datatransfer->data.empty())
	{
		string logdata = datatransfer->data.front();
		datatransfer->data.pop();
		if (datatransfer->fd != -1)
		{
			topic_flag = 0;
		}
		kafkaserver.sendMessage(&datatransfer->kafka_conf, logdata);
		//sendMessageToKafka(logdata.data(), logdata.size());
	} // end of while
	pthread_mutex_unlock(&datatransfer->mutex_data);
	cout << "threadSendKafkaLog exit" <<endl;
	return NULL;
}


int sendLog(DATATRANSFER *datatransfer, string data)
{
	uint32_t hllength;
	uint64_t cm, hlcm;
	string senddata;
	struct timespec ts;
	getTime(&ts);
	cm = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	if (datatransfer->kafka){
		senddata = uintToString(cm) + data;
	}
	else
	{
		hllength = htonl(data.size());
		hlcm = htonl64(cm);
		senddata = string((char *)&hllength, sizeof(uint32_t))
			+ string((char *)&hlcm, sizeof(uint64_t)) + data;
	}

	pthread_mutex_lock(&datatransfer->mutex_data);
	datatransfer->data.push(senddata);
	pthread_mutex_unlock(&datatransfer->mutex_data);

	return 0;
}

int sendLogToKafka(DATATRANSFER *datatransfer, string data)
{
	uint64_t cm;
	struct timespec ts;

	getTime(&ts);
	cm = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	//stringstream ss;
	string timestamp = "";
	/*ss << cm;
	ss >> timestamp;*/
	timestamp = uintToString(cm);
	timestamp = "time=" + timestamp;
	pthread_mutex_lock(&datatransfer->mutex_data);
	datatransfer->data.push(timestamp + "|" + data);
	pthread_mutex_unlock(&datatransfer->mutex_data);

	return 0;
}

string setStrTostring(const set<string> &v)
{
	string str = "";
	if(v.size() == 0)
		return "NULL";
	//char ter;
	//string terminal = string(ter, 1);
	//string terminal = string(1, 1);
	string terminal(1, 1);
	set<string>::const_iterator iter = v.begin();
	str += *iter;
	for(++iter; iter != v.end(); ++iter)
	{
		str += terminal;
		str += *iter;
	}

	return str;
}

string vecStrTostring(vector<string> &v)
{
	string str = "";
	if(v.size() == 0)
		return "NULL";
	//char ter;
	//string terminal = string(ter, 1);
	//string terminal = string(1, 1);
	string terminal(1, 1);
	vector<string>::iterator iter = v.begin();
	str += *iter;
	for(++iter; iter != v.end(); ++iter)
	{
		str +=  terminal;
		str += *iter;	
	}

	return str;
}

void mapToVector(map<uint8_t, string> mappingid, vector<string> &id, vector<uint8_t> &idtype)
{
	map<uint8_t, string>::const_iterator map_it  = mappingid.begin();
	while(map_it != mappingid.end())
	{
		id.push_back(map_it->second);
		idtype.push_back(map_it->first);
		map_it++;
	}
}

string classifyNativeAttr(vector<COM_ASSET_REQUEST_OBJECT> assets)
{
	string data;
	if(assets.size() >0)
	{
		string id;
		string required;
		string type;
		string title_len;
		string image_type;
		string image_w;
		string image_wmin;
		string image_h;
		string image_hmin;
		string image_mimes;
		string data_type;
		string data_len;
		string terminal(1, 1);
		string inner_terminal(1, ',');
		int i = 0;
		id += uintToString(assets[i].id);
		required += uintToString(assets[i].required);
		type += uintToString(assets[i].type);
		title_len += uintToString(assets[i].title.len);
		image_type += uintToString(assets[i].img.type);
		image_w += uintToString(assets[i].img.w);
		image_wmin += uintToString(assets[i].img.wmin);
		image_h += uintToString(assets[i].img.h);
		image_hmin += uintToString(assets[i].img.hmin);
		image_mimes += setIntToString(assets[i].img.mimes, ',');
		data_type += uintToString(assets[i].data.type);
		data_len += uintToString(assets[i].data.len);

		for(i = 1; i <assets.size(); ++i)
		{
			id += terminal + uintToString(assets[i].id);
			required += terminal + uintToString(assets[i].required);
			type += terminal + uintToString(assets[i].type);
			title_len += terminal + uintToString(assets[i].title.len);
			image_type += terminal + uintToString(assets[i].img.type);
			image_w += terminal + uintToString(assets[i].img.w);
			image_wmin += terminal + uintToString(assets[i].img.wmin);
			image_h += terminal + uintToString(assets[i].img.h);
			image_hmin += terminal + uintToString(assets[i].img.hmin);
			image_mimes += terminal + setIntToString(assets[i].img.mimes, ',');
			data_type += terminal + uintToString(assets[i].data.type);
			data_len += terminal + uintToString(assets[i].data.len);
		}
		data += id + '\t' +required + '\t' + type + '\t' + title_len +  '\t' +image_type + '\t'+image_w  + '\t'+ image_wmin + '\t'
			+ image_h + '\t' +image_hmin +'\t'+image_mimes +'\t'+data_type +'\t' +data_len+ '\t';
	}
	else
	{
		//Insert 12 NULL
		for(int i =0; i < 12; i++)
			data += "NULL\t";
	}
	return data;
}

string BidInfo2String(const COM_REQUEST &commreq, const COM_RESPONSE &cresp, uint8_t adx, int errcode)
{
	string data = "";
	string terminal(1, 1);
	string mapid;
	uint8_t bid = 0;
	uint8_t adpos = 0;
	uint16_t advid = 0;

	if (errcode == E_SUCCESS)
		bid = 1;
	data = uintToString(adx) + '\t' + (commreq.id.size() == 0 ? string("NULL") : commreq.id) + '\t' + uintToString(bid) + '\t';
	if (commreq.imp.size() > 0)
	{
		data += (commreq.imp[0].id.size() == 0 ? string("NULL") : commreq.imp[0].id) + '\t'
			+ uintToString(commreq.imp[0].type) + '\t'
			+ uintToString(commreq.imp[0].banner.w) + '\t'
			+ uintToString(commreq.imp[0].banner.h) + '\t'
			+ setIntToString(commreq.imp[0].banner.btype, 1) + '\t'
			+ setIntToString(commreq.imp[0].banner.battr, 1) + '\t'
			+ setIntToString(commreq.imp[0].video.mimes, 1) + '\t'
			+ uintToString(commreq.imp[0].video.display) + '\t'
			+ uintToString(commreq.imp[0].video.minduration) + '\t'
			+ uintToString(commreq.imp[0].video.maxduration) + '\t'
			+ uintToString(commreq.imp[0].video.w) + '\t'
			+ uintToString(commreq.imp[0].video.h) + '\t'
			+ setIntToString(commreq.imp[0].video.battr, 1) + '\t'
			+ uintToString(commreq.imp[0].native.layout) + '\t'
			+ uintToString(commreq.imp[0].native.plcmtcnt) + '\t';

		data += classifyNativeAttr(commreq.imp[0].native.assets);

		data += doubleToString(commreq.imp[0].bidfloor) + '\t'
			+ (commreq.imp[0].bidfloorcur.size() == 0 ? string("NULL") : commreq.imp[0].bidfloorcur) + '\t'
			+ uintToString(commreq.imp[0].ext.instl) + '\t'
			+ uintToString(commreq.imp[0].ext.splash) + '\t';
		adpos = commreq.imp[0].adpos;
		advid = commreq.imp[0].ext.advid;
	}
	else
	{
		//Insert 27 NULL
		data += "NULL\t0\t0\t0\tNULL\tNULL\tNULL\t0\t0\t0\t0\t0\tNULL\t0\t0\t";
		for (int i = 0; i < 9; i++)
			data += "0\t";
		data += "NULL\t0\t0\t0\tNULL\t0\t0\t";
	}

	data += (commreq.app.id.size() == 0 ? string("NULL") : commreq.app.id) + '\t'
		+ (commreq.app.name.size() == 0 ? string("NULL") : commreq.app.name) + '\t'
		+ setIntToString(commreq.app.cat, 1) + '\t'
		+ (commreq.app.bundle.size() == 0 ? "NULL" : commreq.app.bundle) + '\t'
		+ (commreq.app.storeurl.size() == 0 ? "NULL" : commreq.app.storeurl) + '\t';
	vector<uint8_t> didtype, dpidtype;
	vector<string> did, dpid;
	mapToVector(commreq.device.dids, did, didtype);
	mapToVector(commreq.device.dpids, dpid, dpidtype);
	data += vecStrTostring(did) + '\t'
		+ vecIntToString(didtype) + '\t'
		+ vecStrTostring(dpid) + '\t'
		+ vecIntToString(dpidtype) + '\t';

	data += (commreq.device.ua.size() == 0 ? string("NULL") : commreq.device.ua) + '\t'
		+ uintToString(commreq.device.location) + terminal
		+ (commreq.device.ip.size() == 0 ? string("NULL") : commreq.device.ip) + '\t'
		+ doubleToString(commreq.device.geo.lat) + '\t'
		+ doubleToString(commreq.device.geo.lon) + '\t'
		+ uintToString(commreq.device.carrier) + '\t'
		+ uintToString(commreq.device.make) + terminal
		+ (commreq.device.makestr.size() == 0 ? string("NULL") : commreq.device.makestr) + '\t'
		+ (commreq.device.model.size() == 0 ? string("NULL") : commreq.device.model) + '\t'
		+ uintToString(commreq.device.os) + '\t'
		+ (commreq.device.osv.size() == 0 ? string("NULL") : commreq.device.osv) + '\t'
		+ uintToString(commreq.device.connectiontype) + '\t'
		+ uintToString(commreq.device.devicetype) + '\t'
		+ setIntToString(commreq.bcat, 1) + '\t'
		+ setStrTostring(commreq.badv) + '\t'
		+ intToString(commreq.is_recommend) + '\t'
		+ intToString(commreq.at) + '\t'
		+ uintToString(adpos) + '\t';

	data += (commreq.user.id.size() == 0 ? string("NULL") : commreq.user.id) + '\t'
		+ uintToString(commreq.user.gender) + '\t'
		+ intToString(commreq.user.yob) + '\t'
		+ (commreq.user.keywords.size() == 0 ? "NULL" : commreq.user.keywords) + '\t'
		+ doubleToString(commreq.user.geo.lat) + '\t'
		+ doubleToString(commreq.user.geo.lon) + '\t'
		+ (commreq.user.searchkey.size() == 0 ? string("NULL") : commreq.user.searchkey);

	if (errcode == E_SUCCESS && cresp.seatbid.size() > 0 && cresp.seatbid[0].bid.size() > 0){
		mapid = cresp.seatbid[0].bid[0].mapid;
	}
	if (mapid.size() == 0){
		mapid = "NULL";
	}

	data += '\t'
		+ mapid + '\t'
		+ intToString(advid);

	data += '\t' + intToString(commreq.support_deep_link);

	return data;
}

int sendLogdirect(DATATRANSFER *datatransfer,  COM_REQUEST &commreq, uint8_t adx, int errcode)
{
	uint32_t hllength;
	uint64_t cm, hlcm;
	string data = "";
	string terminal(1, 1);
	struct timeval timeofday;
	uint8_t bid = 0;
	uint8_t adpos = 0;
	if(errcode == E_SUCCESS)
		bid = 1;
	gettimeofday(&timeofday, NULL);

	cm = timeofday.tv_sec * 1000 + timeofday.tv_usec / 1000;
	data = uintToString(adx) + '\t' + (commreq.id.size() == 0 ? string("NULL"):commreq.id) + '\t' + uintToString(bid) + '\t';
	if(commreq.imp.size() > 0)
	{
		data += (commreq.imp[0].id.size() == 0 ? string("NULL"):commreq.imp[0].id) + '\t' 
			+ uintToString(commreq.imp[0].type)  + '\t' 
			+ uintToString(commreq.imp[0].banner.w) + '\t' 
			+ uintToString(commreq.imp[0].banner.h)  + '\t' 
			+ setIntToString(commreq.imp[0].banner.btype, 1) + '\t' 
			+ setIntToString(commreq.imp[0].banner.battr, 1) + '\t'
			+ setIntToString(commreq.imp[0].video.mimes, 1) + '\t' 
			+ uintToString(commreq.imp[0].video.display) + '\t'
			+ uintToString(commreq.imp[0].video.minduration) + '\t' 
			+ uintToString(commreq.imp[0].video.maxduration) + '\t' 
			+ uintToString(commreq.imp[0].video.w)  + '\t'
			+ uintToString(commreq.imp[0].video.h) + '\t' 
			+ setIntToString(commreq.imp[0].video.battr, 1) + '\t' 
			+ uintToString(commreq.imp[0].native.layout) + '\t' 
			+ uintToString(commreq.imp[0].native.plcmtcnt) + '\t';

		data += classifyNativeAttr(commreq.imp[0].native.assets);

		data += doubleToString(commreq.imp[0].bidfloor) + '\t'
			+ (commreq.imp[0].bidfloorcur.size() ==0?string("NULL"):commreq.imp[0].bidfloorcur) + '\t' 
			+ uintToString(commreq.imp[0].ext.instl) + '\t'
			+ uintToString(commreq.imp[0].ext.splash) + '\t';
		adpos = commreq.imp[0].adpos;
	}
	else
	{
		//Insert 27 NULL
		data += "NULL\t0\t0\t0\tNULL\tNULL\tNULL\t0\t0\t0\t0\t0\tNULL\t0\t0\t";
		for(int i =0; i < 9; i++)
			data += "0\t";
		data+= "NULL\t0\t0\t0\tNULL\t0\t0\t";
	}

	data += (commreq.app.id.size() == 0?string("NULL"):commreq.app.id) + '\t'
		+ (commreq.app.name.size() == 0?string("NULL"):commreq.app.name)  + '\t'
		+ setIntToString(commreq.app.cat, 1)  + '\t'
		+ (commreq.app.bundle.size() == 0?"NULL":commreq.app.bundle) + '\t' 
		+ (commreq.app.storeurl.size() == 0?"NULL":commreq.app.storeurl) + '\t';
	vector<uint8_t> didtype, dpidtype;
	vector<string> did, dpid;
	mapToVector(commreq.device.dids, did, didtype);
	mapToVector(commreq.device.dpids, dpid, dpidtype);
	data += vecStrTostring(did)  + '\t'
		+ vecIntToString(didtype) + '\t'
		+ vecStrTostring(dpid) + '\t'
		+ vecIntToString(dpidtype) + '\t'; 

	data += (commreq.device.ua.size()== 0?string("NULL"):commreq.device.ua) + '\t'
		+ uintToString(commreq.device.location) + terminal
		+ (commreq.device.ip.size() ==0?string("NULL"):commreq.device.ip) + '\t' 
		+ doubleToString(commreq.device.geo.lat) + '\t' 
		+ doubleToString(commreq.device.geo.lon) + '\t' 
		+ uintToString(commreq.device.carrier) + '\t' 
		+ uintToString(commreq.device.make) + terminal
		+ (commreq.device.makestr.size() ==0?string("NULL"):commreq.device.makestr) + '\t' 
		+ (commreq.device.model.size() == 0?string("NULL"):commreq.device.model) + '\t' 
		+ uintToString(commreq.device.os)+ '\t' 
		+ (commreq.device.osv.size() == 0 ?string("NULL"):commreq.device.osv)  + '\t'
		+ uintToString(commreq.device.connectiontype) + '\t' 
		+ uintToString(commreq.device.devicetype)+ '\t' 
		+ setIntToString(commreq.bcat, 1) + '\t'
		+ setStrTostring(commreq.badv) + '\t'
		+ intToString(commreq.is_recommend) + '\t'
		+ intToString(commreq.at) + '\t'
		+ uintToString(adpos) + '\t';

	data += (commreq.user.id.size() == 0?string("NULL"):commreq.user.id) + '\t'
		+ uintToString(commreq.user.gender) + '\t'
		+ intToString(commreq.user.yob)  + '\t'
		+ (commreq.user.keywords.size() == 0?"NULL":commreq.user.keywords) + '\t'
		+ doubleToString(commreq.user.geo.lat) + '\t' 
		+ doubleToString(commreq.user.geo.lon) + '\t' 
		+ (commreq.user.searchkey.size() == 0?string("NULL"):commreq.user.searchkey);

	//va_cout("send data is :%s", data.c_str());
	hllength = htonl(data.size());
	hlcm = htonl64(cm);
	string logdata = string((char *)&hllength, sizeof(uint32_t))
		+ string((char *)&hlcm, sizeof(uint64_t)) + string(data);
	sendMessage(datatransfer->fd, logdata.data(), logdata.size());
	/*pthread_mutex_lock(&datatransfer->mutex_data);
	datatransfer->data.push(string((char *)&hllength, sizeof(uint32_t))
		+ string((char *)&hlcm, sizeof(uint64_t)) + string(data));
	pthread_mutex_unlock(&datatransfer->mutex_data);
	*/
	return 0;
}

void replaceMacro(string &str, const char *macro, const char *data)
{
	if (data == NULL)
		return;
	while (str.find(macro) < str.size())
		str.replace(str.find(macro), strlen(macro), data);

	return;
}

int selectRedisDB(redisContext *context, uint64_t logid_local, int dbid)
{
	redisReply *reply = NULL;
	string command = "";
	int errcode = E_SUCCESS;

	command = "SELECT " + intToString(dbid);
	reply = (redisReply *)redisCommand(context, command.c_str());
	if(reply != NULL)
	{
        if (reply->type != REDIS_REPLY_STATUS || strcasecmp(reply->str, "OK"))
        {
			errcode = E_REDIS_COMMAND;
			writeLog(logid_local, LOGDEBUG, "Execute redisCommand %s ,err:%d", command.c_str(), reply->type);
        }
	}
	else
	{
	//	if (!context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed, err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int existRedisKey(redisContext *context, uint64_t logid_local, string key)
{
	redisReply *reply = NULL;
	string command = "";
	int errcode = E_UNKNOWN;

	command = "EXISTS " + key;
	reply = (redisReply *)redisCommand(context, command.c_str());
	if(reply != NULL)
	{
		if (reply->integer == 1)
		{
			errcode = E_SUCCESS;
		}
		else 
		{
			errcode = E_REDIS_KEY_NOT_EXIST;
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed" + " err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int existRedisSetKey(redisContext *context, uint64_t logid_local, string key, string member)
{
	redisReply *reply = NULL;
	string command = "";
	int errcode = E_UNKNOWN;

	if (context == NULL)
	{
	   errcode = E_REDIS_CONNECT_INVALID;
	   writeLog(logid_local, LOGDEBUG, "existRedisSetKey redisContext is null");
	   goto exit;
	}
	command = "SISMEMBER " + key + " " + member;
	reply = (redisReply *)redisCommand(context, command.c_str());
	if(reply != NULL)
	{
		if (reply->integer == 1)
		{
			errcode = E_SUCCESS;
			//writeLog(logid_local, LOGINFO, "key: " + key + ", member: " + member + " existed!");
		}
		else//reply->integer == 0
		{
			errcode = E_REDIS_KEY_NOT_EXIST;
			//writeLog(logid_local, LOGINFO, "key: " + key + ", member: " + member + " not existed!");
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed" + " err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int getRedisValue(redisContext *context, uint64_t logid_local, string key, string &value)
{
	int errcode = E_SUCCESS;
	redisReply *reply = NULL;
	string command = "";
	
	command = "GET " + key;
	//大部分场景key都是存在的
	/*errcode = existRedisKey(context, logid_local, key);
	if (errcode != E_SUCCESS)
	{
		goto exit;
	}*/

	reply = (redisReply *)redisCommand(context, command.c_str());
	if (reply != NULL) 
	{
		if(reply->type != REDIS_REPLY_NIL)
		{
			value = reply->str;
		}
		else
		{
			errcode = E_REDIS_KEY_NOT_EXIST;
			writeLog(logid_local, LOGINFO, string(command) + " not exist");
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed" + " err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int setexRedisValue(redisContext *context, uint64_t logid_local, string key, int time, string value)
{
	int errcode = E_SUCCESS;
	redisReply *reply = NULL;
	string command = "";

	command = "SETEX " + key + " " + intToString(time) + " " + value;
	reply = (redisReply *)redisCommand(context, command.c_str());
	if (reply != NULL)
	{
		if (reply->type != REDIS_REPLY_STATUS || strcasecmp(reply->str, "OK"))
		{
			errcode = E_REDIS_COMMAND;
			writeLog(logid_local, LOGDEBUG, "Execute redisCommand %s ,err:%d", command.c_str(), reply->type);
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed, err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int hgetallRedisValue(redisContext *context, uint64_t logid_local, string key, map<string, string> &m_value)
{
	int errcode = E_SUCCESS;
	redisReply *reply = NULL;
	string command = "";

	command = "HGETALL " + key;
	reply = (redisReply *)redisCommand(context, command.c_str());
		
	if (reply != NULL)
	{
		if (reply->type != REDIS_REPLY_ARRAY)
		{
			writeLog(logid_local, LOGINFO, "Execute redisCommand " + string(command) + " failed!");
			errcode = E_REDIS_COMMAND;
			goto exit;
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed" + " err:" + string(context->errstr));
			goto exit;
		}
	}

	cflog(logid_local, LOGWARNING, "%s, --at %s/%d... %s has %d elements", 
		"id", __FUNCTION__, __LINE__, key.c_str(), reply->elements);

	for (int i = 0; i < reply->elements; i += 2) 
	{
 		cflog(logid_local, LOGWARNING, "%s, --at %s/%d...element[%d]- key:%s, value:%s", 
 			"id", __FUNCTION__, __LINE__, 
 			i, reply->element[i]->str, reply->element[i + 1]->str);

		if ((reply->element[i]->type == REDIS_REPLY_STRING) && (reply->element[i + 1]->type == REDIS_REPLY_STRING))
		{
			m_value.insert(make_pair(reply->element[i]->str, reply->element[i + 1]->str));
		}
	}

	//if (m_value.size() == 0)
	//{
	//	bRes = false;
	//	goto exit;
	//}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int hgetRedisValue(redisContext *context, uint64_t logid_local, string key, string field, string &value)
{
	int errcode = E_SUCCESS;
	redisReply *reply = NULL;
	string command = "";

	command = "HGET " + key + " " + field;
	reply = (redisReply *)redisCommand(context, command.c_str());
	if (reply != NULL)
	{
		if (reply->type != REDIS_REPLY_NIL)
		{
			value = reply->str;
		}
		else
		{
			errcode = E_REDIS_KEY_NOT_EXIST;
			writeLog(logid_local, LOGINFO, string(command) + " not exist");
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed" + " err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int expireRedisValue(redisContext *context, uint64_t logid_local, string key, int time)
{
	int errcode = E_SUCCESS;
	redisReply *reply = NULL;
	string command = "";

	command = "EXPIRE " + key + " " + intToString(time);
	reply = (redisReply *)redisCommand(context, command.c_str());
	if(reply != NULL)
	{
		if (reply->type != REDIS_REPLY_INTEGER)
		{
			errcode = E_REDIS_COMMAND;
			writeLog(logid_local, LOGDEBUG, "Execute redisCommand %s ,err:%d", command.c_str(), reply->type);
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed, err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}


int hsetRedisValue(redisContext *context, uint64_t logid_local, string key, string field, string value)
{
	int errcode = E_UNKNOWN;
	redisReply *reply = NULL;
	string command = "";

	command = "HSET " + key + " " + field + " " + value;
	reply = (redisReply *)redisCommand(context, command.c_str());
	if (reply != NULL)
	{
		if ((reply->type == REDIS_REPLY_INTEGER) && ((reply->integer == 0) || (reply->integer == 1)))
		{
			errcode = E_SUCCESS;
		}
		else
		{
			errcode = E_REDIS_COMMAND;
			writeLog(logid_local, LOGINFO, "Execute redisCommand " + string(command) + " failed");
		}
	}
	else
	{
	//	if (context->err)
		{
			errcode = E_REDIS_CONNECT_INVALID;
			writeLog(logid_local, LOGDEBUG, "redisContext is invaild, Execute " + string(command) + " failed" + " err:" + string(context->errstr));
		}
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int get_TARGET_object(json_t *root, uint8_t adx, DEVICE_TARGET &device_target, APP_TARGET &app_target)
{
	int errcode = E_SUCCESS;
	json_t *label , *labelinner;
	labelinner = label = NULL;
	if(root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "device", JSON_OBJECT))
	{
		labelinner = label->child;
		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "flag", JSON_NUMBER))
		{
			uint32_t flag = atoi(label->child->text);
			flag &= TARGET_DEVICE_DISABLE_ALL;
			device_target.flag = flag;
			if (flag != TARGET_ALLOW_ALL && flag != TARGET_DEVICE_DISABLE_ALL)
			{
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "regioncode", JSON_ARRAY))
					jsonGetIntegerSet(label, device_target.regioncode);
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "connectiontype", JSON_ARRAY))
					jsonGetIntegerSet(label, device_target.connectiontype);
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "os", JSON_ARRAY))
					jsonGetIntegerSet(label, device_target.os);
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "carrier", JSON_ARRAY))
					jsonGetIntegerSet(label, device_target.carrier);
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "devicetype", JSON_ARRAY))
					jsonGetIntegerSet(label, device_target.devicetype);
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "make", JSON_ARRAY))
					jsonGetIntegerSet(label, device_target.make);
			}
		}
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "app", JSON_OBJECT))
	{
		labelinner = label->child;

		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "id", JSON_ARRAY))
		{
			json_t *idtmp;
			idtmp = label->child->child;
			while(idtmp)
			{
				uint8_t redis_adx = 0;
				if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "adx", JSON_NUMBER))
				{
					redis_adx = atoi(label->child->text);
					if(redis_adx == adx)
					{
						uint32_t flag = 0;
						if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "flag", JSON_NUMBER))
						{
							flag = atoi(label->child->text);
							flag &= TARGET_APP_ID_CAT_WBLIST;
							app_target.id.flag = flag ;
						}

						if (flag != TARGET_ALLOW_ALL)
						{
							if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "wlist", JSON_ARRAY))
								jsonGetStringSet(label, app_target.id.wlist);
							if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "blist", JSON_ARRAY))
								jsonGetStringSet(label, app_target.id.blist);
						}
						break;
					}
				}
				idtmp = idtmp->next;
			}
		}
		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "cat", JSON_OBJECT))
		{
			json_t *cattmp;
			cattmp = label->child;
			if(cattmp != NULL)
			{
				uint32_t flag = 0;
				if(JSON_FIND_LABEL_AND_CHECK(cattmp, label, "flag", JSON_NUMBER))
				{
					flag = atoi(label->child->text);
					flag &= TARGET_APP_ID_CAT_WBLIST;
					app_target.cat.flag = flag ;
				}
				if (flag != TARGET_ALLOW_ALL)
				{
					if(JSON_FIND_LABEL_AND_CHECK(cattmp, label, "wlist", JSON_ARRAY))
						jsonGetIntegerSet(label, app_target.cat.wlist);
					if(JSON_FIND_LABEL_AND_CHECK(cattmp, label, "blist", JSON_ARRAY))
						jsonGetIntegerSet(label, app_target.cat.blist);
				}
			}
		}
	}
exit:
	return 	E_SUCCESS ;
}

bool get_SCENE_TARGET(json_t *root, SCENE_TARGET &scene_target)
{
	json_t *label , *labelinner;
	labelinner = label = NULL;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "scene", JSON_OBJECT))
	{
		labelinner = label->child;
		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "flag", JSON_NUMBER))
		{
			uint32_t flag = atoi(label->child->text);
			flag &= TARGET_SCENE_MASK;
			scene_target.flag = flag;
			if (flag != TARGET_ALLOW_ALL)
			{
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "length", JSON_NUMBER))
					scene_target.length = atoi(label->child->text);

				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "loc", JSON_ARRAY))
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						/*if(JSON_FIND_LABEL_AND_CHECK(temp, label, "lat", JSON_NUMBER))
						{

						}
						if(JSON_FIND_LABEL_AND_CHECK(temp, label, "lon", JSON_NUMBER))
						{

						}*/
						if(JSON_FIND_LABEL_AND_CHECK(temp, label, "id", JSON_STRING))
						{
							scene_target.loc_set.insert(string(label->child->text, scene_target.length));
						}
						
						temp = temp->next;
					}
				}
			}
		}
	}

	return true;
}

int get_GROUP_TARGET_object(redisContext *context, uint8_t adx, uint64_t logid_local, string groupid, GROUP_TARGET &target)
{
	int errcode = E_UNKNOWN;
	string value = "";
	json_t *root = NULL, *label, *labelinner;

	errcode = getRedisValue(context, logid_local, "dsp_groupid_target_" + string(groupid), value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "dsp_groupid_target_" + string(groupid) + " does not exist!");
		goto exit;
	}

	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		writeLog(logid_local, LOGINFO, "get_GROUP_TARGET_object: root is NULL");
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "groupid", JSON_STRING))
		target.groupid = label->child->text;

	get_TARGET_object(root, adx, target.device_target, target.app_target);

	get_SCENE_TARGET(root, target.scene_target);

exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

int get_ADX_TARGET_object(redisContext *context, uint8_t adx, uint64_t logid_local, ADX_TARGET &target)
{
	int errcode = E_SUCCESS;
	string value = "";
	json_t *root, *label;
	char command[MIN_LENGTH] = "";

	root = label = NULL;
	if(adx == ADX_MAX)
	{
		errcode = E_INVALID_PARAM_ADX;
		goto exit;
	}
	
	sprintf(command, "dsp_adx_target_%02d", adx);
	errcode = getRedisValue(context, logid_local, command, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}
	//there is no adx target, valid
	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, string(command) + " does not exist!");
		goto exit;
	}

	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		writeLog(logid_local, LOGINFO, "get_ADX_TARGET_object: root is NULL");
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_NUMBER))
	{
		target.adx = atoi(label->child->text);
		if(target.adx != adx)
		{
			errcode = E_INVALID_PARAM_ADX;
			goto exit;
		}
		get_TARGET_object(root, adx, target.device_target, target.app_target);
	}

exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

int getWBList(redisContext *context, uint64_t logid_local, string groupid, WBLIST &wblist)
{
	int errcode = E_SUCCESS;
	string value = "";
	json_t *root = NULL, *label;

	errcode = getRedisValue(context, logid_local, "dsp_groupid_wblist_" + groupid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "dsp_groupid_wblist_" + string(groupid) + " does not exist!");
		goto exit;
	}

	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		writeLog(logid_local, LOGINFO, "getWBList: root is NULL");
		goto exit;
	}
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "groupid", JSON_STRING))
		wblist.groupid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "relationid", JSON_STRING))
		wblist.relationid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "ratio", JSON_NUMBER))
		wblist.ratio = atof(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "mprice", JSON_NUMBER))
		wblist.mprice = atof(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "whitelist", JSON_ARRAY))
		jsonGetIntegerSet(label, wblist.whitelist);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "blacklist", JSON_ARRAY))
		jsonGetIntegerSet(label, wblist.blacklist);
	json_free_value(&root);

exit:
	return errcode;
}

int getCreativeInfo(redisContext *context, uint8_t adx, uint64_t logid_local, map<string, uint8_t>img_imptype, string creativeid, CREATIVEOBJECT &creative)
{
	int errcode = E_SUCCESS;
	string value = "";
	json_t *root = NULL, *label, *temp;

	errcode = getRedisValue(context, logid_local, "dsp_mapid_" + creativeid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "dsp_mapid_" + string(creativeid) + " does not exist!");
		goto exit;
	}

	//va_cout("creative info is :%s" value.c_tr());
	json_parse_document(&root, value.c_str());

	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "type", JSON_NUMBER))
		creative.type = atoi(label->child->text);

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "w", JSON_NUMBER))
		creative.w = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "h", JSON_NUMBER))
		creative.h = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "ftype", JSON_NUMBER))
		creative.ftype = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "sourceurl", JSON_STRING))
		creative.sourceurl = label->child->text;

	if(creative.type == ADTYPE_FEEDS)
	{
		IMAG_OBJECT imgs;
		init_IMG_OBJECT(imgs);
		if(JSON_FIND_LABEL_AND_CHECK(root, label, "h", JSON_NUMBER))
			imgs.h = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(root, label, "w", JSON_NUMBER))
			imgs.w = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(root, label, "ftype", JSON_NUMBER))
			imgs.ftype = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(root, label, "sourceurl", JSON_STRING))
			imgs.sourceurl = label->child->text;
		if(imgs.h != 0 && imgs.w != 0 && imgs.sourceurl != "")
		{
			creative.imgs.push_back(imgs);
		}
	}
	if(creative.type == ADTYPE_IMAGE && adx != ADX_MAX)
	{
		char imgsize[MIN_LENGTH] = "";
		sprintf(imgsize, "%dx%d", creative.w, creative.h);
		if(img_imptype.count(imgsize) == 0)
		{
			errcode = E_UNKNOWN;
			goto exit;
		}
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "mapid", JSON_STRING))
		creative.mapid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "groupid", JSON_STRING))
		creative.groupid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "price_adx", JSON_ARRAY))
	{
		json_t *temp = label->child->child;
		while(temp != NULL)
		{
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "adx", JSON_NUMBER))
			{
				int redis_adx = atoi(label->child->text);
				if(redis_adx == adx)
				{
					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "price", JSON_NUMBER))
						creative.price = atof(label->child->text);
					goto outLoop;
				}
			}
			temp = temp->next;
		}
	}
	//else if(JSON_FIND_LABEL_AND_CHECK(root, label, "price", JSON_NUMBER))
	//{
	//	creative.price = atof(label->child->text);
	//}
outLoop:	
	if(creative.price > -0.0000001 && creative.price < 0.000001 && adx != ADX_MAX)
	{
		va_cout("not find creative.price: %lf %s!!!", creative.price, creative.mapid.c_str());
		errcode = E_REDIS_INVALID_CREATIVE_INFO;
		goto exit;
	}
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "adid", JSON_STRING))
		creative.adid = label->child->text;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "ctype", JSON_NUMBER))
		creative.ctype = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "bundle", JSON_STRING))
		creative.bundle = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "apkname", JSON_STRING))
		creative.apkname = label->child->text;
	
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "curl", JSON_STRING))
		creative.curl = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "securecurl", JSON_STRING))
		creative.securecurl = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "landingurl", JSON_STRING))
		creative.landingurl = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "cmonitorurl", JSON_ARRAY))
		jsonGetStringArray(label, creative.cmonitorurl);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "imonitorurl", JSON_ARRAY))
		jsonGetStringArray(label, creative.imonitorurl);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "monitorcode", JSON_STRING))
		creative.monitorcode = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "deeplink", JSON_STRING))
		creative.deeplink = label->child->text;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "cid", JSON_STRING))
		creative.cid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "crid", JSON_STRING))
		creative.crid = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "attr", JSON_ARRAY))
		jsonGetIntegerSet(label, creative.attr);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "badx", JSON_ARRAY))
	{
		jsonGetIntegerSet(label, creative.badx);
		if(adx != ADX_MAX)
		{
			if(creative.badx.count(adx))
			{
				errcode = E_REDIS_INVALID_CREATIVE_INFO;
				goto exit;
			}
		}
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "icon", JSON_OBJECT))
	{
		json_t *icon_obj = label->child;
		if(JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "ftype", JSON_NUMBER))
			creative.icon.ftype = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "w", JSON_NUMBER))
			creative.icon.w = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "h", JSON_NUMBER))
			creative.icon.h = atoi(label->child->text);
		if(JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "sourceurl", JSON_STRING))
			creative.icon.sourceurl = label->child->text;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "imgs", JSON_ARRAY))
	{
		json_t *img_obj = label->child->child;
		while(img_obj != NULL)
		{
			IMAG_OBJECT imgs;
			init_IMG_OBJECT(imgs);
			if(JSON_FIND_LABEL_AND_CHECK(img_obj, label, "h", JSON_NUMBER))
				imgs.h = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(img_obj, label, "w", JSON_NUMBER))
				imgs.w = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(img_obj, label, "ftype", JSON_NUMBER))
				imgs.ftype = atoi(label->child->text);
			if(JSON_FIND_LABEL_AND_CHECK(img_obj, label, "sourceurl", JSON_STRING))
				imgs.sourceurl = label->child->text;
			img_obj = img_obj->next;
			if(imgs.h != 0 && imgs.w != 0 && imgs.sourceurl != "")
			{
				creative.imgs.push_back(imgs);
			}
		}
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "title", JSON_STRING))
		creative.title = label->child->text;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "description", JSON_STRING))
		creative.description = label->child->text;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "rating", JSON_NUMBER))
		creative.rating = atoi(label->child->text);

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "ctatext", JSON_STRING))
		creative.ctatext = label->child->text;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "exts", JSON_ARRAY))
	{
			json_t *extstmp;
			extstmp = label->child->child;
			while(extstmp)
			{
				int redis_adx = 0;
				char *text = NULL;
				if(JSON_FIND_LABEL_AND_CHECK(extstmp, label, "adx", JSON_NUMBER))
				{
					redis_adx = atoi(label->child->text);
					if(redis_adx == adx)
					{
						if(JSON_FIND_LABEL_AND_CHECK(extstmp, label, "id", JSON_STRING))
							creative.ext.id = label->child->text;
						if(JSON_FIND_LABEL_AND_CHECK(extstmp, label, "ext", JSON_OBJECT))
						{
							json_tree_to_string(label->child, &text);
							if(text != NULL)
							{
								creative.ext.ext = text;
								free(text);
								text = NULL;
							}
						}
					}
				}
				extstmp =extstmp->next;
			}
	}
exit:
	if(root != NULL)
		json_free_value(&root);

	return errcode;
}

int getGroupSmartBidInfo(redisContext *context, uint8_t adx, uint64_t logid_local, string groupid, GROUP_SMART_BID_INFO &grsmartbidinfo)
{
	if(adx == ADX_MAX)
		return E_INVALID_PARAM_ADX;

	string value = "";
	int errcode = E_SUCCESS;

	json_t *root = NULL, *label;

	errcode = getRedisValue(context, logid_local, "dsp_groupid_smart_" + groupid, value);
	if(errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		writeLog(logid_local, LOGINFO, "dsp_groupid_smart_" + groupid + " does not exist!");
		errcode = E_REDIS_NO_CONTENT;
		goto exit;
	}

	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
		writeLog(logid_local, LOGINFO, "get_GROUP_TARGET_object: root is NULL");
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	errcode = E_INVALID_PARAM_ADX;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "smart", JSON_ARRAY))
	{
		json_t *temp = label->child->child;
		while(temp != NULL)
		{
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "adx", JSON_NUMBER))
			{
				uint8_t redis_adx = atoi(label->child->text);
				if(redis_adx == adx)
				{
					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "flag", JSON_NUMBER))
					{
						grsmartbidinfo.flag = atoi(label->child->text);
					}
					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "maxratio", JSON_NUMBER))
					{
						grsmartbidinfo.maxratio = atof(label->child->text);
					}
					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "minratio", JSON_NUMBER))
					{
						grsmartbidinfo.minratio = atof(label->child->text);
					}
					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "interval", JSON_NUMBER))
					{
						grsmartbidinfo.interval = atoi(label->child->text);
					}
					errcode = E_SUCCESS;
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

/*int getLocation(const char *ip, uint64_t logid_local)
{
	string senddata;
	char host[MIN_LENGTH], *recvdata = NULL;
	int errorcode;
	uint32_t length = 0;
	char location[10] = {0};
	int fd;

	fd = connectServer(LOCATION_SERVER, LOCATION_PORT);
	if (fd == -1)
	{
		writeLog(logid_local, LOGINFO, "getLocation: " + string(ip) + " connect location server fail!");
		return 0;
	}

	sprintf(host, "%s:%d", LOCATION_SERVER, LOCATION_PORT);
	senddata = "GET /geo.php?ip=" + string(ip) + " HTTP/1.1\r\nUser-Agent: Mozilla/5.0\r\nHost: "
		+ string(host) + "\r\n\r\n";
	sendMessage(fd, senddata.data(), senddata.size());
	errorcode = receiveMessage(fd, &recvdata, &length);
	disconnectServer(fd);
	if (recvdata != NULL)
	{
		memcpy(location, recvdata + 3, 6);
		free(recvdata);
		return atoi(location);
	}
	else
		return 0;
}*/

void clear_ADXTEMPLATE(ADXTEMPLATE &adxinfo)
{
	adxinfo.adx  = adxinfo.ratio = 0;
	//adxinfo.aurl = adxinfo.nurl = "";
	adxinfo.iurl.clear();
	adxinfo.cturl.clear();
	adxinfo.adms.clear();
}

int getGroupInfo(redisContext *context, uint8_t adx, uint64_t logid_local, string groupid, GROUPINFO &ginfo)
{
	int errcode = E_SUCCESS;
	string value = "";
	json_t *root = NULL, *label, *temp;

	errcode = getRedisValue(context, logid_local, "dsp_groupid_info_" + groupid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "dsp_groupid_info_" + string(groupid) + " does not exist!");
		goto exit;
	}
	//va_cout("GROUPINFO is %s", value.c_str());
	json_parse_document(&root, value.c_str());

	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_ARRAY))
	{
		jsonGetIntegerSet(label, ginfo.adx);
		if(adx != ADX_MAX)
		{
			if(!ginfo.adx.count(adx))
			{
				errcode = E_INVALID_PARAM_ADX;
				goto exit;
			}
		}
	}
	else
	{
		errcode = E_INVALID_PARAM_ADX;
		goto exit;
	}
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "cat", JSON_ARRAY))
		jsonGetIntegerSet(label, ginfo.cat);

	if(adx == ADX_MAX)
		goto out;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "advcat", JSON_NUMBER))
		ginfo.advcat = atoi(label->child->text);

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "auctiontype", JSON_ARRAY))
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			uint8_t redis_adx = 0;
			if(JSON_FIND_LABEL_AND_CHECK(temp, label, "adx", JSON_NUMBER))
			{
				redis_adx = atoi(label->child->text);
				if(redis_adx == adx)
				{
					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "at", JSON_NUMBER))
					{
						ginfo.at = atoi(label->child->text);
					}

					if(JSON_FIND_LABEL_AND_CHECK(temp, label, "dealid", JSON_STRING))
					{
						ginfo.dealid = (label->child->text);
					}

					goto out;
				}
			}
			temp = temp->next;
		}
	}
out:
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "adomain", JSON_STRING))
		ginfo.adomain = label->child->text;
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "redirect", JSON_NUMBER))
		ginfo.redirect = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "is_secure", JSON_NUMBER))
		ginfo.is_secure = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "effectmonitor", JSON_NUMBER))
		ginfo.effectmonitor = atoi(label->child->text);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "exts", JSON_ARRAY))
	{
		json_t *extstmp = NULL;
		extstmp = label->child->child;
		while(extstmp)
		{
			uint8_t redis_adx = 0;
			string advid;
			if(JSON_FIND_LABEL_AND_CHECK(extstmp, label, "adx", JSON_NUMBER))
			{
				redis_adx = atoi(label->child->text);
				if(redis_adx == adx)
				{
					if(JSON_FIND_LABEL_AND_CHECK(extstmp, label, "advid", JSON_STRING))
						ginfo.ext.advid = label->child->text;	
				}
			}
				
			extstmp = extstmp->next;
		}
	}

exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

int getADVCAT(redisContext *context, uint64_t logid_local, map<string, double> &advcat)
{
	int errcode = E_SUCCESS;
	string value;
	json_t *root = NULL, *child;

	errcode = getRedisValue(context, logid_local, "dsp_relation_advcat", value);
	if(errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "dsp_relation_advcat does not exist!");
		goto exit;
	}
	//va_cout("GROUPINFO is %s", value.c_str());
	json_parse_document(&root, value.c_str());

	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	child = root->child;
	while(child != NULL)
	{
		if(child->type == JSON_STRING)
		{
			if(child->child != NULL && child->child->type == JSON_NUMBER)
			{
				advcat.insert(make_pair(child->text, atof(child->child->text)));
			}
		}
		child = child->next;
	}

exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

int getUerCLKADVCATinfo(redisContext *context, uint64_t logid_local, string userid, USER_CLKADVCAT_INFO &userclkadvcat)
{
	int errcode = E_SUCCESS;
	string value;
	json_t *root = NULL, *label;

	userclkadvcat.userid = userid;

	errcode = getRedisValue(context, logid_local, "dsp_user_click_advcat_" + userid, value);
	if(errcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	if (value.size() == 0)
	{
		errcode = E_REDIS_NO_CONTENT;
		writeLog(logid_local, LOGINFO, "dsp_user_click_advcat_" + userid + "does not exist!");
		goto exit;
	}
	//va_cout("GROUPINFO is %s", value.c_str());
	json_parse_document(&root, value.c_str());

	if (root == NULL)
	{
		errcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "clickadvcat", JSON_ARRAY))
		jsonGetIntegerSet(label, userclkadvcat.clkadvcat);

	if(userclkadvcat.clkadvcat.size() == 0)
		errcode = E_REDIS_NO_CONTENT;
	
exit:
	if(root != NULL)
		json_free_value(&root);
	return errcode;
}

void va_cout(const char *fmt, ...)
{
#ifndef DEBUG
	return;
#endif
	int n, size = 256;
	char *writecontent, *np;
	va_list ap;

	if ((writecontent = (char *)malloc(size)) == NULL)
		return;

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(writecontent, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = (char *)realloc (writecontent, size)) == NULL) {
			free(writecontent);
			writecontent = NULL;
			break;
		} else {
			writecontent = np;
		}
	}
	if(writecontent != NULL)
	{
		cout<< writecontent << endl;
		free(writecontent);
	}
	return;
}

void record_cost(uint64_t logid, bool is_print_time, const char *id, const char *stage, timespec ts1)
{
	if (!is_print_time)
		return;

	struct timespec ts2;
	long lasttime;

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

	if (lasttime > 0)
		cflog(logid, LOGMAX, "%s, %s time: %d", id, stage, lasttime);
}

string md5String(MD5_CTX hash_ctx, string original)
{
	char output[33] = "";
	char temp[3] = "";
	unsigned char hash_ret[16];  

	MD5_Update(&hash_ctx, original.c_str(), original.size());  
	MD5_Final(hash_ret, &hash_ctx); 
	for(int i = 0; i < 32 ;i++)
	{
		sprintf(temp, "%x%x", (hash_ret[i++/2] >> 4) & 0xf, (hash_ret[i/2]) & 0xf);
		strcat(output, temp);
	}
	return output;
}

void replaceAdmasterMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url, string extid)
{
	if(url.find("__AndroidID__") != string::npos)
	{
		string os = "";
		string androididmd5 = "";
		string imeimd5 = "";
		string macmd5 = "";

		if (device.dids.count(DID_MAC_MD5))
		{
			macmd5 = device.dids[DID_MAC_MD5];
		}
		else if(device.dids.count(DID_MAC) > 0)
		{		//md5加密mac
			string mac = device.dids[DID_MAC];
			replaceMacro(mac, ":", "");
			replaceMacro(mac, "-", "");
			transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
			macmd5 = md5String(hash_ctx, mac);
		}
		if(macmd5.size() > 0)
		{
			replaceMacro(url, "__MAC__", macmd5.c_str());
		}

		if (device.ip != "")
		{
			replaceMacro(url, "__IP__", device.ip.c_str());
		}

		if (device.model != "")
		{
			int en_len = 0;
			char *model = urlencode(device.model.c_str(), device.model.size(), &en_len);
			replaceMacro(url, "__TERM__", model);
			free(model);
		}
		else
		{
			replaceMacro(url, "__TERM__", "unknown");
		}
		switch(device.os)
		{
		case OS_IOS:
			{
				os = "1";
				//md5加密idfa
				if(device.dpids.count(DPID_IDFA) > 0)
				{
					string idfa = device.dpids[DPID_IDFA];
					if(idfa.size() > 0)
						replaceMacro(url, "__IDFA__", idfa.c_str());
				}
				break;
			}
		case OS_ANDROID:
			{
				os = "0";
				if(device.dpids.count(DPID_ANDROIDID_MD5) > 0)
				{
					androididmd5 = device.dpids[DPID_ANDROIDID_MD5];

				}
				else if(device.dpids.count(DPID_ANDROIDID) > 0)
				{
					string androidid = device.dpids[DPID_ANDROIDID];

					androididmd5 = md5String(hash_ctx, androidid);

				}
				if(androididmd5.size() > 0)
				{
					replaceMacro(url, "__AndroidID__", androididmd5.c_str());
				}

				if(device.dids.count(DID_IMEI_MD5) > 0)
				{
					imeimd5 = device.dids[DID_IMEI_MD5];
				}
				else if(device.dids.count(DID_IMEI) > 0)
				{
					string imei = device.dids[DID_IMEI];

					imeimd5 = md5String(hash_ctx, imei);

				}
				if(imeimd5.size() > 0)
				{
					replaceMacro(url, "__IMEI__", imeimd5.c_str());
				}

				if(extid.size() > 0)
				{
					if(macmd5.size() ==0 && androididmd5.size() == 0 && imeimd5.size() == 0)
					{
						replaceMacro(url, "__IDFA__", extid.c_str());
					}
				}

				break;
			}
		case OS_WINDOWS:
			{
				os = "2";
				break;
			}
		default:
			os = "3";
		}
		replaceMacro(url, "__OS__", os.c_str());
	}
	if(url.find(",h") != string::npos)
	{
		url.insert(url.find(",h")+ 2, extid);
		//url += extid;
	}
	return;
}


void replaceNielsenMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url)
{
	string os = "";
	string androididmd5 = "";
	string imeimd5 = "";
	string macmd5 = "";

	if (device.dids.count(DID_MAC_MD5))
	{
		macmd5 = device.dids[DID_MAC_MD5];
	}
	else if(device.dids.count(DID_MAC) > 0)
	{		//md5加密mac
		string mac = device.dids[DID_MAC];
		replaceMacro(mac, ":", "");
		replaceMacro(mac, "-", "");
		transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
		macmd5 = md5String(hash_ctx, mac);
	}

	if(macmd5.size() > 0)
	{
		replaceMacro(url, "__MAC__", macmd5.c_str());
	}

	switch(device.os)
	{
	case OS_IOS:
		{
			os = "1";
			//md5加密idfa
			if(device.dpids.count(DPID_IDFA) > 0)
			{
				string idfa = device.dpids[DPID_IDFA];
				if(idfa.size() > 0)
					replaceMacro(url, "__IDFA__", idfa.c_str());
			}
			break;
		}
	case OS_ANDROID:
		{
			os = "0";
			if(device.dpids.count(DPID_ANDROIDID_MD5) > 0)
			{
				androididmd5 = device.dpids[DPID_ANDROIDID_MD5];
			}
			else if(device.dpids.count(DPID_ANDROIDID) > 0)
			{
				string androidid = device.dpids[DPID_ANDROIDID];
				androididmd5 = md5String(hash_ctx, androidid);
			}
			if(androididmd5.size() > 0)
			{
				replaceMacro(url, "__ANDROIDID__", androididmd5.c_str());
			}

			if(device.dids.count(DID_IMEI_MD5) > 0)
			{
				imeimd5 = device.dids[DID_IMEI_MD5];
			}
			else if(device.dids.count(DID_IMEI) > 0)
			{
				string imei = device.dids[DID_IMEI];

				imeimd5 = md5String(hash_ctx, imei);

			}
			if(imeimd5.size() > 0)
			{
				replaceMacro(url, "__IMEI__", imeimd5.c_str());
			}
			break;
		}
	case OS_WINDOWS:
		{
			os = "2";
			break;
		}
	default:
		os = "3";
	}
	replaceMacro(url, "__OS__", os.c_str());
	return;
}

void replaceMiaozhenMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url)
{
	string os = "";
    string imeimd5 = "";
    string macmd5 = "";

    if (device.ip != "")
    {
        replaceMacro(url, "__IP__", device.ip.c_str());
    }

	if (device.dids.count(DID_MAC_MD5))
	{
		macmd5 = device.dids[DID_MAC_MD5];
	}
    else if(device.dids.count(DID_MAC) > 0)
    {        //md5加密mac
        string mac = device.dids[DID_MAC];
        replaceMacro(mac, ":", "");
        replaceMacro(mac, "-", "");
        transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
        macmd5 = md5String(hash_ctx, mac);
    }
    if(macmd5.size() > 0)
    {
        replaceMacro(url, "__MAC__", macmd5.c_str());
    }

    if(device.dids.count(DID_IMEI_MD5) > 0)
    {
        imeimd5 = device.dids[DID_IMEI_MD5];
    }
    else if(device.dids.count(DID_IMEI) > 0)
    {
        string imei = device.dids[DID_IMEI];

        imeimd5 = md5String(hash_ctx, imei);
    }
    if(imeimd5.size() > 0)
    {
        replaceMacro(url, "__IMEI__", imeimd5.c_str());
    }

    switch(device.os)
    {
    case OS_IOS:
        {
            os = "1";
            if(device.dpids.count(DPID_IDFA) > 0)
            {
                string idfa = device.dpids[DPID_IDFA];
                if(idfa.size() > 0)
                    replaceMacro(url, "__IDFA__", idfa.c_str());
            }
            break;
        }
    case OS_ANDROID:
        {
            os = "0";

            if (device.dpids.count(DPID_ANDROIDID) > 0)
            {
                string android = device.dpids[DPID_ANDROIDID];
                if (android.size() > 0)
                    replaceMacro(url, "__ANDROIDID1__", android.c_str());
            }
            else if(device.dpids.count(DPID_ANDROIDID_MD5) > 0)
            {
                string androididmd5 = device.dpids[DPID_ANDROIDID_MD5];
                if (androididmd5.size() > 0)
                    replaceMacro(url, "__ANDROIDID__", androididmd5.c_str());
            }
            break;
        }
    case OS_WINDOWS:
        {
            os = "2";
            break;
        }
    default:
        os = "3";
    }
    replaceMacro(url, "__OS__", os.c_str());
    return;
}

void replaceGimcMonitor(MD5_CTX hash_ctx, IN COM_DEVICEOBJECT &device, INOUT string &url)
{
	string macmd5 = "";
	string imeimd5 = "";
	string os = "";
	int devicetype = device.devicetype == 0 ? 3 : device.devicetype;
	int carrier = device.carrier == 0 ? 4 : device.carrier;
	int connectiontype = 4;
	switch (device.connectiontype)
	{
	case CONNECTION_UNKNOWN:
		connectiontype = 4;
		break;
	case CONNECTION_WIFI:
		connectiontype = 3;
		break;
	case CONNECTION_CELLULAR_2G:
		connectiontype = 1;
		break;
	case CONNECTION_CELLULAR_3G:
		connectiontype = 2;
		break;
	case CONNECTION_CELLULAR_4G:
		connectiontype = 5;
		break;
	}

	if (device.ip != "")
	{
		replaceMacro(url, "__IP__", device.ip.c_str());
	}

	replaceMacro(url, "__DEVICETYPE__", intToString(devicetype).c_str());
	replaceMacro(url, "__ISPID__", intToString(carrier).c_str());
	replaceMacro(url, "__NETWORKMANNERID__", intToString(connectiontype).c_str());

	if (device.model.size() > 0)
	{
		replaceMacro(url, "__DEVICE__", device.model.c_str());
	}

	if (device.dids.count(DID_MAC_MD5))
	{
		macmd5 = device.dids[DID_MAC_MD5];
	}
	else if (device.dids.count(DID_MAC) > 0)
	{        //md5加密mac
		string mac = device.dids[DID_MAC];
		replaceMacro(mac, ":", "");
		replaceMacro(mac, "-", "");
		transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
		macmd5 = md5String(hash_ctx, mac);
	}
	if (macmd5.size() > 0)
	{
		replaceMacro(url, "__MAC__", macmd5.c_str());
	}

	if (device.dids.count(DID_IMEI_MD5) > 0)
	{
		imeimd5 = device.dids[DID_IMEI_MD5];
	}
	else if (device.dids.count(DID_IMEI) > 0)
	{
		string imei = device.dids[DID_IMEI];
		imeimd5 = md5String(hash_ctx, imei);
	}
	if (imeimd5.size() > 0)
	{
		replaceMacro(url, "__IMEI__", imeimd5.c_str());
	}

	switch (device.os)
	{
	case OS_ANDROID:
	{
		os = "0";

		if (device.dpids.count(DPID_ANDROIDID_MD5) > 0)
		{
			string androididmd5 = device.dpids[DPID_ANDROIDID_MD5];
			if (androididmd5.size() > 0)
				replaceMacro(url, "__ANDROIDID__", androididmd5.c_str());
		}
		break;
	}
	case OS_IOS:
	{
		os = "1";
		if (device.dpids.count(DPID_IDFA) > 0)
		{
			string idfa = device.dpids[DPID_IDFA];
			if (idfa.size() > 0)
				replaceMacro(url, "__IDFA__", idfa.c_str());
		}
		break;
	}
	case OS_WINDOWS:
	{
		os = "2";
		break;
	}
	default:
		os = "3";
	}
	replaceMacro(url, "__OS__", os.c_str());

	return;
}

// 0, success
int set_timer(IN int second)
{
    struct itimerval tick;
    tick.it_value.tv_sec = second;
    tick.it_value.tv_usec = 0;
    tick.it_interval.tv_sec = 0;
    tick.it_interval.tv_usec = 0;
    int ret = setitimer(ITIMER_REAL, &tick, NULL);

    return ret;
}

// 0, success
int set_timer_day(IN uint64_t logid)
{
    int second = DAYSECOND - getCurrentTimeSecond();
	syslog(LOG_INFO, "set timer before second:%d", second);
	if (second < 60)
		second = 60;
    int ret = set_timer(second);
    if ( ret != 0)
    {
        cflog(logid,LOGDEBUG,"Set timer error:%s", strerror(errno) );
    }
    return ret;
}

bool connect_log_server(uint64_t logid, const char *ip, uint16_t port, bool *run_flag, DATATRANSFER *datatransfer)
{
	datatransfer->logid = logid; 
	datatransfer->server = string(ip);
	datatransfer->port = port;
	datatransfer->run_flag = run_flag;
	datatransfer->write = 0;
	datatransfer->threadlog = -1;
	pthread_mutex_init(&datatransfer->mutex_data, 0);
	
	datatransfer->fd = connectServer(datatransfer->server.c_str(), datatransfer->port);
	if (datatransfer->fd == -1)
	{
		writeLog(logid, LOGERROR, "Connect log server " + datatransfer->server + " failed!");
		return false;
	}
	pthread_create(&datatransfer->threadlog, NULL, threadSendLog, datatransfer);

	return true;
}

bool connect_log_server(uint64_t logid, KAFKA_CONF *kafka_conf, bool *run_flag, DATATRANSFER *datatransfer)
{
	datatransfer->logid = logid;
	datatransfer->server = "";
	datatransfer->port = 0;
	datatransfer->run_flag = run_flag;
	datatransfer->write = 0;
	datatransfer->kafka_conf.broker_list = kafka_conf->broker_list;
	datatransfer->kafka_conf.topic = kafka_conf->topic;

	KafkaServer &kafkaserver = KafkaServer::getInstance();
	datatransfer->fd = kafkaserver.connectServer(&datatransfer->kafka_conf);
	pthread_mutex_init(&datatransfer->mutex_data, 0);
	pthread_create(&datatransfer->threadlog, NULL, threadSendKafkaLog, datatransfer);
	if (datatransfer->fd == -1)
	{
		writeLog(logid, LOGERROR, "Connect log server " + string(datatransfer->kafka_conf.broker_list) + " failed!");
		return false;
	}
	return true;
}

void disconnect_log_server(DATATRANSFER *datatransfer)
{
	if (datatransfer != NULL)
	{
		if (datatransfer->fd != -1)
		{
			disconnectServer(datatransfer->fd);
			datatransfer->fd = -1;
		}	
		pthread_mutex_destroy(&datatransfer->mutex_data);
	}
	return;
}

void disconnect_log_server(DATATRANSFER *datatransfer, bool is_kafka)
{
	if (datatransfer != NULL)
	{
		if (datatransfer->fd != -1)
		{
			if (is_kafka == false)
			{
				disconnectServer(datatransfer->fd);
			}
			else
			{
				KafkaServer &kafkaserver = KafkaServer::getInstance();
				kafkaserver.disconnectServer(&datatransfer->kafka_conf);
				//disconnectServerToKafka();
			}
		}
		pthread_mutex_destroy(&datatransfer->mutex_data);
	}
   return;
}

void replace_url(IN string bid, IN string mapid, IN string dpid, IN int deviceidtype, INOUT string &url)
{
	replaceMacro(url,"#MAPID#",mapid.c_str());
	replaceMacro(url,"#BID#",bid.c_str());
	replaceMacro(url, "#DEVICEIDTYPE#", intToString(deviceidtype).c_str());
	replaceMacro(url,"#DEVICEID#",dpid.c_str());
	return;
}

// 一个函数两个功能
string cal_impm(const COM_IMPRESSIONOBJECT *imp, int &width, int &height)
{
	string ret;
	if (!imp){
		return ret;
	}

	char buf[64] = {0};
	if (imp->type == IMP_BANNER)
	{
		sprintf(buf, "%d,%d", imp->ext.instl, imp->ext.splash);
		width = imp->banner.w;
		height = imp->banner.h;
	}
	else if (imp->type == IMP_NATIVE)
	{
		int icon_width = 0, icon_height = 0;
		bool has_get_img = false;
		int icon_num = 0, mainimg_num = 0, title_num = 0, desc_num = 0;
		const vector<COM_ASSET_REQUEST_OBJECT> &assets = imp->native.assets;
		for (int i = 0; i < assets.size(); i++)
		{
			const COM_ASSET_REQUEST_OBJECT &asset = assets[i];
			if (asset.required == 0){
				continue;
			}

			if (asset.type == NATIVE_ASSETTYPE_TITLE){
				title_num++;
			}
			else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
			{
				if (asset.img.type == ASSET_IMAGETYPE_ICON)
				{
					icon_num++;
					icon_width = asset.img.w;
					icon_height = asset.img.h;
				}
				else if (asset.img.type == ASSET_IMAGETYPE_MAIN)
				{
					mainimg_num++;
					if (!has_get_img)
					{
						width = asset.img.w;
						height = asset.img.h;
						has_get_img = true;
					}
				}
			}
			else if (asset.type == NATIVE_ASSETTYPE_DATA && asset.data.type == ASSET_DATATYPE_DESC)
			{
				desc_num++;
			}
		}
		sprintf(buf, "%d,%d,%d,%d", icon_num, mainimg_num, title_num, desc_num);

		if (!has_get_img)
		{
			width = icon_width;
			height = icon_height;
		}
	}
	else if (imp->type == IMP_VIDEO)
	{
		width = imp->video.w;
		height = imp->video.h;
	}

	ret = buf;

	return ret;
}

// "bid=&at=&mapid=&impid=&impt=&impm=&w=&h=&deviceid=&deviceidtype=&appid=&nw=&os=&gp=&tp=&mb=&md=&op=&ds="
string get_trace_url_par(IN const COM_REQUEST &req, IN const COM_RESPONSE &cresp)
{
	int w = 0, h = 0;
	const COM_BIDOBJECT *bid = NULL;
	const COM_IMPRESSIONOBJECT *imp = NULL; 

	if (cresp.seatbid.size() > 0 && cresp.seatbid[0].bid.size() > 0){
		bid = &cresp.seatbid[0].bid[0];
	}

	if (req.imp.size() > 0){
		imp = &req.imp[0];
	}

	string ret = "bid=";
	ret += urlencode(req.id);
	ret += "&at=";
	ret += intToString(req.at);
	ret += "&mapid=";
	if (bid){
		ret += bid->mapid;
	}
	ret += "&impid=";
	if (imp){
		ret += urlencode(imp->id);
	}
	ret += "&impt=";
	if (imp){
		ret += intToString(imp->type);
	}
	ret += "&impm=";
	if (imp){
		ret += cal_impm(imp, w, h);
	}

	ret += "&ip=";
	ret += req.device.ip;

	ret += "&w=";
	ret += intToString(w);
	ret += "&h=";
	ret += intToString(h);
	ret += "&deviceid=";
	ret += req.device.deviceid;
	ret += "&deviceidtype=";
	ret += intToString(req.device.deviceidtype);
	ret += "&appid=";
	ret += urlencode(req.app.id);
	ret += "&nw=";
	ret += intToString(req.device.connectiontype);
	ret += "&os=";
	ret += intToString(req.device.os);
	ret += "&gp=";
	ret += intToString(req.device.location);
	ret += "&tp=";
	ret += intToString(req.device.devicetype);
	ret += "&mb=";
	ret += intToString(req.device.make);
	ret += "&md=";
	ret += urlencode(req.device.model);
	ret += "&op=";
	ret += intToString(req.device.carrier);
	ret += "&ds=";
	if (bid)
	{
		const char *flag_dsp = "0";
		if (bid->groupid_type == "pap"){
			flag_dsp = "1";
		}
		ret += flag_dsp;
	}

	return ret;
}

void replace_icn_url(IN string bid, IN string mapid, IN COM_DEVICEOBJECT &device, IN string appid, INOUT string &url)
{
	char *temp_appid = NULL;
	char *temp_model = NULL;
	string appid_str = "";
	string model_str = "";
	int temp_appid_len = 0;
	int temp_model_len = 0;
	if(appid != "")
	{
		temp_appid = urlencode(appid.c_str(), strlen(appid.c_str()), &temp_appid_len);
		appid_str = temp_appid;
	}
	if(device.model != "")
	{
		temp_model = urlencode(device.model.c_str(), strlen(device.model.c_str()), &temp_model_len);
		model_str = temp_model;
	}

	replaceMacro(url,"#MAPID#",mapid.c_str());
	replaceMacro(url,"#BID#",bid.c_str());
	replaceMacro(url, "#DEVICEIDTYPE#", intToString(device.deviceidtype).c_str());
	replaceMacro(url,"#DEVICEID#",device.deviceid.c_str());

	url += "&appid=" + appid_str + "&nw="+ intToString(device.connectiontype) + "&os=" +
			intToString(device.os) + "&tp=" + intToString(device.devicetype) +
			"&reqip=" + device.ip + "&gp=" + intToString(device.location) + "&mb=" +
			intToString(device.make) + "&op=" + intToString(device.carrier) + "&md=" +
			model_str;

	if(temp_appid != NULL)
		free(temp_appid);
	if(temp_model != NULL)
		free(temp_model);
	return;
}

void replaceCurlParam(IN string bid, IN COM_DEVICEOBJECT &cdevice, IN string mapid, INOUT string &url)
{
	replaceMacro(url, "#BID#", bid.c_str());
	replaceMacro(url, "#MAPID#", mapid.c_str());
	//gp
	replaceMacro(url, "#GP#", intToString(cdevice.location).c_str());
	replaceMacro(url, "#OP#", intToString(cdevice.carrier).c_str());
	replaceMacro(url, "#MB#", intToString(cdevice.make).c_str());
	replaceMacro(url, "#NW#", intToString(cdevice.connectiontype).c_str());
	replaceMacro(url, "#TP#", intToString(cdevice.devicetype).c_str());
	return;
}

void replace_https_url(IN ADXTEMPLATE &adxinfo, IN int is_secure)
{
	string str1 = "";
	string str2 = "";
	if(is_secure == 0)
	{
		str1 = "https:";
		str2 = "http:";
	}
	else if(is_secure == 1)
	{
		str1 = "http:";
		str2 = "https:";
	}
	replaceMacro(adxinfo.nurl,str1.c_str(),str2.c_str());
	replaceMacro(adxinfo.aurl,str1.c_str(),str2.c_str());
	int i;
	for(i = 0; i < adxinfo.iurl.size(); ++i)
	{
		replaceMacro(adxinfo.iurl[i],str1.c_str(),str2.c_str());
	}
	for(i = 0; i < adxinfo.cturl.size(); ++i)
	{
		replaceMacro(adxinfo.cturl[i],str1.c_str(),str2.c_str());
	}

	return;
}
