#include "stdafx.h"
#include "util.h"

#include	"hiredis.h"
#define		NO_QFORKIMPL //这一行必须加才能正常使用;

#include	"Win32_Interop\win32fixes.h"
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:"4099")

extern char *redis_ip, *redis_ip_r;
extern int	redis_port, redis_port_r;
redisContext *ctx = NULL, *ctx_r = NULL;
using namespace std;

int write_Intelligent(char *groupid, char *key, vector<INTELLIGENT_INFO> in_info);

char* KS_UNICODE_to_UTF8(const LPCWSTR wcsString)
{
	if (wcsString == NULL)
		return NULL;

	int nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wcsString, -1, NULL, 0, NULL, NULL);
	char *pUTF8 = (char *)malloc(nLen + 1);
	ZeroMemory(pUTF8, nLen + 1);
	nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wcsString, -1, pUTF8, nLen, NULL, NULL);

	return pUTF8;
}

WCHAR* KS_UTF8_to_UNICODE(const char *szUTF8)
{
	if (szUTF8 == NULL)
		return L"";

	int nLen = MultiByteToWideChar(CP_UTF8, 0, szUTF8, -1, NULL, 0);
	WCHAR *pWstr = (WCHAR*)malloc(sizeof(WCHAR)* (nLen + 1));
	ZeroMemory(pWstr, sizeof(WCHAR)* (nLen + 1));
	MultiByteToWideChar(CP_UTF8, 0, szUTF8, -1, pWstr, nLen);

	return pWstr;
}

DWORD IniReadStrByNameA(char *file, char *section, char *key, char *szOut)
{
	char szPath[MAX_PATH] = "";
	GetModuleFileNameA(NULL, szPath, sizeof(szPath));
	(strrchr(szPath, '\\'))[1] = 0;
	strcat(szPath, file);

	return GetPrivateProfileStringA(section, key, NULL, szOut, MAX_PATH, szPath);
}

BOOL IniWriteStrByNameA(char *file, char *section, char *key, char *str)
{
	char szPath[MAX_PATH] = "";
	GetModuleFileNameA(NULL, szPath, sizeof(szPath));
	(strrchr(szPath, '\\'))[1] = 0;
	strcat(szPath, file);
	return  WritePrivateProfileStringA(section, key, str, szPath);
}

BOOL IniWriteStrByNameW(WCHAR *file, WCHAR *section, WCHAR *key, WCHAR *str)
{
	WCHAR szPath[MAX_PATH] = L"";
	GetModuleFileNameW(NULL, szPath, sizeof(szPath));
	(wcsrchr(szPath, '\\'))[1] = 0;
	wcscat(szPath, file);
	return  WritePrivateProfileStringW(section, key, str, szPath);
}

void json_insert_string(json_t *entry, const char *label, const char * value)
{
	json_t *jlabel = json_new_string(label);
	json_t *jvalue = json_new_string(value);

	json_insert_child(jlabel, jvalue);
	json_insert_child(entry, jlabel);
}

void json_insert_int(json_t *entry, const char *label, int value)
{
	json_t *jlabel, *jvalue;
	char str[64] = "";

	jlabel = json_new_string(label);
	sprintf(str, "%d", value);
	jvalue = json_new_number(str);
	json_insert_child(jlabel, jvalue);
	json_insert_child(entry, jlabel);
}

void json_insert_double(json_t *entry, const char *label, double value)
{
	json_t *jlabel, *jvalue;
	char str[MIN_LENGTH] = "";

	jlabel = json_new_string(label);
	sprintf(str, "%f", value);
	jvalue = json_new_number(str);
	json_insert_child(jlabel, jvalue);
	json_insert_child(entry, jlabel);
}

int existRedisKey(redisContext *context, char *key)
{
	redisReply *reply;
	char command[MIN_LENGTH] = "";
	int exist = 0;

	sprintf(command, "EXISTS %s", key);
	reply = (redisReply *)redisCommand(context, command);
	if (reply != NULL)
	{
		exist = reply->integer;
		freeReplyObject(reply);
	}

	return exist;
}

int TTLRedisKey(redisContext *context, char *key)
{
	redisReply *reply;
	char command[MIN_LENGTH] = "";
	int exist = 0;

	sprintf(command, "TTL %s", key);
	reply = (redisReply *)redisCommand(context, command);
	if (reply != NULL)
	{
		exist = reply->integer;
		freeReplyObject(reply);
	}

	return exist;
}

bool setexRedisValue(redisContext *context, char *key, long lefetime, char *value)
{
	bool bRes = false;
	char command[MID_LENGTH] = "";

	sprintf(command, "SETEX %s %ld %s", key, lefetime, value);

	redisReply *reply = (redisReply *)redisCommand(context, command);
	if ((reply != NULL) && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str, "OK") == 0))
	{
		bRes = true;
		goto exit;
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return bRes;
}

char *getRedisValue(redisContext *context, char *key, char *value)
{
	char command[MIN_LENGTH] = "";

	if (existRedisKey(context, key) == 0)
		return "";

	sprintf(command, "GET %s", key);
	redisReply *reply = (redisReply *)redisCommand(context, command);
	if ((reply != NULL) && (reply->type != REDIS_REPLY_NIL))
	{
		strcpy(value, reply->str);
		goto exit;
	}

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return value;
}

int read_Intelligent(char *groupid, char *key, int &interval, double &minratio, double &maxratio, int &adx, int &flag, double &left_time)
{
	char *text = NULL;
	json_t *root = NULL, *label = NULL;
	int errcode = E_SUCCESS;
	char value[MID_LENGTH] = "";
	vector <INTELLIGENT_INFO> in_info;

	getRedisValue(ctx, key, value);
	if (strlen(value) == 0)
	{
		errcode = E_READ_REDIS_EMPTY;
		goto exit;
	}

	json_parse_document(&root, value);
	if (root == NULL)
	{
		errcode = E_READ_REDIS_NO_CONTEXT;
		goto exit;
	}

	if (((label = json_find_first_label(root, "groupid")) != NULL) && (label->child->type == JSON_STRING))
	{
		if (!strcmp(groupid, label->child->text))
			printf("Find groupid..");
		else
			return false;
	}

	if ((label = json_find_first_label(root, "smart")) != NULL)
	{
		json_t *t1 = label->child->child, *t2 = NULL, *lable_intel = NULL;
		int j_adx = 0, j_flag = 0, j_interval = 30,
			j_minprice = 5, j_maxprice = 10;

		while (t1 != NULL)
		{
			if ((t2 = json_find_first_label(t1, "adx")) != NULL)
			{
				j_adx = atoi(t2->child->text);
				
				if (flag == -1)
				{
					long left_time_temp = TTLRedisKey(ctx, key);
					if (left_time_temp != -1)
						left_time = (double)left_time_temp / 86400;

					if (j_adx == adx)
					{
						errcode = E_READ_REDIS_FIND_ADX;

						if ((t2 = json_find_first_label(t1, "flag")) != NULL)
							flag = atoi(t2->child->text);
						if ((t2 = json_find_first_label(t1, "interval")) != NULL)
							interval = atoi(t2->child->text);
						if ((t2 = json_find_first_label(t1, "minratio")) != NULL)
							minratio = atof(t2->child->text);
						if ((t2 = json_find_first_label(t1, "maxratio")) != NULL)
							maxratio = atof(t2->child->text);

						goto exit;
					}
				}
				else
				{
					if (j_adx == adx)
					{
						errcode = E_READ_REDIS_FIND_ADX;
						if ((t2 = json_find_first_label(t1, "flag")) != NULL)
						{
							char *new_flag = (char *)malloc(10 * sizeof(char));
							memset(new_flag, 0, 10 * sizeof(char));
							sprintf(new_flag, "%d", flag);
							free(t2->child->text);
							t2->child->text = new_flag;
						}

						if ((t2 = json_find_first_label(t1, "interval")) != NULL)
						{
							char *new_interval = (char *)malloc(10 * sizeof(char));
							memset(new_interval, 0, 10 * sizeof(char));
							sprintf(new_interval, "%d", interval);
							free(t2->child->text);
							t2->child->text = new_interval;
						}

						if ((t2 = json_find_first_label(t1, "minratio")) != NULL)
						{
							char *new_minprice = (char *)malloc(10 * sizeof(char));
							memset(new_minprice, 0, 10 * sizeof(char));
							sprintf(new_minprice, "%f", minratio);
							free(t2->child->text);
							t2->child->text = new_minprice;
						}

						if ((t2 = json_find_first_label(t1, "maxratio")) != NULL)
						{
							char *new_maxprice = (char *)malloc(10 * sizeof(char));
							memset(new_maxprice, 0, 10 * sizeof(char));
							sprintf(new_maxprice, "%f", maxratio);
							free(t2->child->text);
							t2->child->text = new_maxprice;
						}
					}
				}
			}

			t1 = t1->next;
		}

		if (flag != -1)
		{
			json_t *t3 = NULL;

			if (errcode != E_READ_REDIS_FIND_ADX)
			{
				if ((label = json_find_first_label(root, "smart")) != NULL)
				{
					json_t *t3 = label->child;
					lable_intel = json_new_object();
					json_insert_int(lable_intel, "adx", adx);
					json_insert_int(lable_intel, "flag", flag);
					json_insert_int(lable_intel, "interval", interval);
					json_insert_double(lable_intel, "minratio", minratio);
					json_insert_double(lable_intel, "maxratio", maxratio);
					json_insert_child(t3, lable_intel);
					json_insert_child(root, t3);
				}
			}
			
			long left_time_temp = TTLRedisKey(ctx, key);
			if (left_time_temp != -1)
				left_time = (double)left_time_temp;

			json_tree_to_string(root, &text);
			if (!setexRedisValue(ctx, key, left_time, text))
				errcode = E_REDIS_EXECUTE_FAILED;
		}
	}

exit:
	if (text != NULL)
		free(text);

	return errcode;
}

int write_Intelligent(char *groupid, char *key, vector<INTELLIGENT_INFO> in_info)
{
	int errcode = E_SUCCESS;
	char *text = NULL;
	json_t *root = NULL, *label_intelligent = NULL, *label_array = NULL, *lable_intel = NULL, *label_adx = NULL, *label_flag = NULL;

	root = json_new_object();
	json_insert_string(root, "groupid", groupid);
	label_intelligent = json_new_string("smart");
	label_array = json_new_array();
	
	lable_intel = json_new_object();
	json_insert_int(lable_intel, "adx", in_info[0].adx);
	json_insert_int(lable_intel, "flag", in_info[0].flag);
	json_insert_int(lable_intel, "interval", in_info[0].interval);
	json_insert_double(lable_intel, "minratio", in_info[0].minratio);
	json_insert_double(lable_intel, "maxratio", in_info[0].maxratio);
	json_insert_child(label_array, lable_intel);

	json_insert_child(label_intelligent, label_array);
	json_insert_child(root, label_intelligent);

	json_tree_to_string(root, &text);
	json_free_value(&root);

	if (!setexRedisValue(ctx, key, in_info[0].valid_time, text))
		errcode = E_REDIS_EXECUTE_FAILED;

	if (text != NULL)
		free(text);

	return errcode;
}

int redis_operion(TCHAR *groupid, INTELLIGENT_INFO &v_info)
{
	char key[MIN_LENGTH] = "";
	char *szgroupid = NULL;
	redisReply *reply = NULL;
	vector<INTELLIGENT_INFO> in_info;

	int errcode = E_SUCCESS;

	szgroupid = KS_UNICODE_to_UTF8(groupid);

	sprintf(key, "dsp_groupid_smart_%s", szgroupid);

	if (existRedisKey(ctx, key) == 0)
	{
		errcode = E_EXIST_GROUPID;
		if (v_info.flag != -1)
		{
			INTELLIGENT_INFO info;
			info.adx = v_info.adx;
			info.flag = v_info.flag;
			info.interval = v_info.interval;
			info.minratio = v_info.minratio;
			info.maxratio = v_info.maxratio;
			info.valid_time = v_info.valid_time * 86400;
			in_info.push_back(info);
			errcode = write_Intelligent(szgroupid, key, in_info);
		}

		goto exit;
	}

	errcode = read_Intelligent(szgroupid, key, v_info.interval, v_info.minratio, v_info.maxratio, v_info.adx, v_info.flag, v_info.valid_time);

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int get_aver_price(char *get_groupid_info, int adx, vector <AVER_PRICE> &averprice, double &left_time)
{
	int errcode = E_SUCCESS, finish_time_r = -1;
	json_t *root = NULL;
	char aprice_keys[MID_LENGTH] = "", outvalue[MID_LENGTH], mhour[MINI_LENGTH] = "", mratio[MID_LENGTH] = "",
		finish_tm[MIN_LENGTH] = "", left_tm[MIN_LENGTH] = "";
	double ratio = 0;
	long keys_time = 0;

	SYSTEMTIME sys;
	GetLocalTime(&sys);

	sprintf(mhour, "%02d", sys.wHour);
	sprintf(left_tm, "dsp_groupid_smart_%s", get_groupid_info);
	sprintf(mratio, "dsp_groupid_smart_%02d_%s_%02d", adx, get_groupid_info, sys.wDay);
	sprintf(aprice_keys, "HGETALL dsp_groupid_price_%02d_%02d_%s", adx, sys.wDay, get_groupid_info);
	sprintf(finish_tm, "HGET dsp_group_frequency_%02d_%02d %s", adx, sys.wDay, get_groupid_info);

	keys_time = TTLRedisKey(ctx, left_tm);
	if (keys_time != -1)
		left_time = (double )keys_time / 86400;

	getRedisValue(ctx, mratio, outvalue);
	if (strlen(outvalue) != 0)
	{
		json_t *label = NULL;

		json_parse_document(&root, outvalue);
		if (root != NULL)
		{
			if ((label = json_find_first_label(root, get_groupid_info)) != NULL)
			{
				json_t *t1 = label->child, *t2 = NULL;

				if ((t2 = json_find_first_label(t1, mhour)) != NULL)
					ratio = atof(t2->child->text);
			}
		}
	}

	redisReply *reply_finish = (redisReply *)redisCommand(ctx_r, finish_tm);
	if ((reply_finish != NULL) && (reply_finish->type != REDIS_REPLY_NIL))
	{
		char szvalue[MAX_LENGTH] = "", szimp[MINI_LENGTH] = "", szclk[MINI_LENGTH] = "";
		json_t *f_root = NULL, *f_label = NULL;
		
		strcpy(szvalue, reply_finish->str);
		json_parse_document(&f_root, szvalue);
		if (f_root != NULL)
		{
			sprintf(szimp, "s_imp_t_%02d", sys.wHour);
			sprintf(szclk, "s_clk_t_%02d", sys.wHour);

			if ((f_label = json_find_first_label(f_root, szimp)) != NULL)
				finish_time_r = atoi(f_label->child->text);
			else if ((f_label = json_find_first_label(f_root, szclk)) != NULL)
				finish_time_r = atoi(f_label->child->text);
		}
	}

	redisReply *reply = (redisReply *)redisCommand(ctx_r, aprice_keys);
	if (reply == NULL || reply->type != REDIS_REPLY_ARRAY)
	{
		errcode = E_READ_REDIS_EMPTY;
		goto exit;
	}

	for (int i = 0; i < reply->elements; i += 2)
	{
		if ((reply->element[i]->type == REDIS_REPLY_STRING) && (reply->element[i + 1]->type == REDIS_REPLY_STRING))
		{
			AVER_PRICE aprice;
			int w = 0, h = 0;
			double outprice = 0.0, rprice = 0.0;
			char mapid_keys[MID_LENGTH] = "", mapid_value[MAX_LENGTH] = "", size[MINI_LENGTH] = "";
			json_t *aroot = NULL, *alabel = NULL, *oroot = NULL, *olabel = NULL;

			strcpy(aprice.mapid, reply->element[i]->str);
			json_parse_document(&aroot, reply->element[i + 1]->str);
			if (aroot != NULL)
			{
				if ((alabel = json_find_first_label(aroot, mhour)) != NULL)
					rprice = atof(alabel->child->text);
 				else
					continue;
			}

			sprintf(mapid_keys, "dsp_mapid_%s", aprice.mapid);
			getRedisValue(ctx, mapid_keys, mapid_value);
			if (strlen(mapid_value) != 0)
			{
				json_parse_document(&oroot, mapid_value);
				if (oroot != NULL)
				{
					if ((olabel = json_find_first_label(oroot, "price_adx")) != NULL)
					{
						json_t *t1 = olabel->child->child, *t2 = NULL;

						while (t1 != NULL)
						{
							if ((t2 = json_find_first_label(t1, "adx")) != NULL)
							{
								if (adx == atoi(t2->child->text))
								{
									if ((t2 = json_find_first_label(t1, "price")) != NULL)
									{
										outprice = atof(t2->child->text);
										break;
									}
								}
							}

							t1 = t1->next;
						}
					}
					
					if (outprice == 0.0)
					{
						if ((olabel = json_find_first_label(oroot, "price")) != NULL)
							outprice = atof(olabel->child->text);
					}

					if ((olabel = json_find_first_label(oroot, "w")) != NULL)
						w = atoi(olabel->child->text);
					if ((olabel = json_find_first_label(oroot, "h")) != NULL)
						h = atoi(olabel->child->text);

					sprintf(size, "%d x %d", w, h);

					aprice.real_price = rprice;
					aprice.finish_time = finish_time_r;
					aprice.out_price = (1 + ratio) * outprice;
					strcpy(aprice.size, size);

					averprice.push_back(aprice);
				}
			}	
		}
	}
		
exit:
	return errcode;
}

// double get_keys_left_time(char *groupid)
// {
// 	char left_tm[MIN_LENGTH] = "";
// 	long keys_time = 0.0;
// 	double left_time = 0.0;
// 
// 	sprintf(left_tm, "dsp_groupid_smart_%s", groupid);
// 	keys_time = TTLRedisKey(ctx, left_tm);
// 	if (keys_time != -1)
// 		left_time = (double)keys_time / 86400;
// 
// 	return left_time;
// }

int redis_connecting()
{
	int errcode = E_SUCCESS;

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	ctx = redisConnectWithTimeout(redis_ip, redis_port, timeout);
	if ((ctx == NULL) || (ctx->err != 0))
	{
		errcode = E_REDIS_CONNECT_FAILD;
		goto exit;
	}

	ctx_r = redisConnectWithTimeout(redis_ip_r, redis_port_r, timeout);
	if ((ctx_r == NULL) || (ctx_r->err != 0))
	{
		errcode = E_REDIS_CONNECT_FAILD;
		goto exit;
	}

exit:
	return errcode;
}

int redis_deconnect()
{
	int errcode = E_SUCCESS;

	if (ctx != NULL)
	{
		redisFree(ctx);
		ctx = NULL;
	}
		
	if (ctx_r != NULL)
	{
		redisFree(ctx_r);
		ctx_r = NULL;
	}

	return errcode;
}