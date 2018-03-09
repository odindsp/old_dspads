#include "stdafx.h"
#include "util.h"

#include	"hiredis.h"
#define		NO_QFORKIMPL //这一行必须加才能正常使用;

#include	"Win32_Interop\win32fixes.h"
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:"4099")

extern TCHAR *redis_ip;
extern int redis_port;
redisContext *ctx = NULL;
using namespace std;

int write_Intelligent(char *key, int adx, int err_level, int print_time);

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

int existRedisKey(char *key)
{
	redisReply *reply;
	char command[150] = "";
	int exist = 0;

	sprintf(command, "EXISTS %s", key);
	reply = (redisReply *)redisCommand(ctx, command);
	if (reply != NULL)
	{
		exist = reply->integer;
		freeReplyObject(reply);
	}

	return exist;
}

bool setexRedisValue(char *key, char *value)
{
	bool bRes = false;
	char command[10000] = "";

	sprintf(command, "SET %s %s", key, value);

	redisReply *reply = (redisReply *)redisCommand(ctx, command);
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

char *getRedisValue(char *key, char *value)
{
	char command[150] = "";

	if (existRedisKey(key) == 0)
		return "";

	sprintf(command, "GET %s", key);
	redisReply *reply = (redisReply *)redisCommand(ctx, command);
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

int read_Intelligent(char *key, int flag, int adx, int &err_level, int &print_time)
{
	char *text = NULL;
	json_t *root = NULL, *label = NULL;
	int errcode = E_SUCCESS;
	char value[1000] = "";

	getRedisValue(key, value);
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

	if ((label = json_find_first_label(root, "dsp_log_conf")) != NULL)
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
					if (j_adx == adx)
					{
						if ((t2 = json_find_first_label(t1, "ferr_level")) != NULL)
							err_level = atoi(t2->child->text);
						if ((t2 = json_find_first_label(t1, "is_print_time")) != NULL)
							print_time = atoi(t2->child->text);

						goto exit;
					}
				}
				else
				{
					if (j_adx == adx)
					{
						errcode = E_READ_REDIS_FIND_ADX;
						if ((t2 = json_find_first_label(t1, "ferr_level")) != NULL)
						{
							char *new_err = (char *)malloc(10 * sizeof(char));
							memset(new_err, 0, 10 * sizeof(char));
							sprintf(new_err, "%d", err_level);
							t2->child->text = new_err;
						}

						if ((t2 = json_find_first_label(t1, "is_print_time")) != NULL)
						{
							char *new_p_time = (char *)malloc(10 * sizeof(char));
							memset(new_p_time, 0, 10 * sizeof(char));
							sprintf(new_p_time, "%d", print_time);
							t2->child->text = new_p_time;
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
				if ((label = json_find_first_label(root, "dsp_log_conf")) != NULL)
				{
					json_t *t3 = label->child;
					lable_intel = json_new_object();
					json_insert_int(lable_intel, "adx", adx);
					json_insert_int(lable_intel, "ferr_level", err_level);
					json_insert_int(lable_intel, "is_print_time", print_time);
					json_insert_child(t3, lable_intel);
					json_insert_child(root, t3);
				}
			}
			
			json_tree_to_string(root, &text);
			if (!setexRedisValue(key, text))
				errcode = E_REDIS_EXECUTE_FAILED;
			else
				errcode = E_SUCCESS;
		}
	}

exit:

	if (text != NULL)
		free(text);

	return errcode;
}

int write_Intelligent(char *key, int adx, int err_level, int print_time)
{
	int errcode = E_SUCCESS;
	char *text = NULL;
	json_t *root = NULL, *label_log = NULL, *label_array = NULL, *lable_list = NULL, *label_adx = NULL, *label_flag = NULL;

	root = json_new_object();
	label_log = json_new_string("dsp_log_conf");
	label_array = json_new_array();

	{
		lable_list = json_new_object();
		json_insert_int(lable_list, "adx", adx);
		json_insert_int(lable_list, "ferr_level", err_level);
		json_insert_int(lable_list, "is_print_time", print_time);
		json_insert_child(label_array, lable_list);
	}

	json_insert_child(label_log, label_array);
	json_insert_child(root, label_log);

	json_tree_to_string(root, &text);
	json_free_value(&root);

	if (!setexRedisValue(key, text))
		errcode = E_REDIS_EXECUTE_FAILED;

	if (text != NULL)
		free(text);

	return errcode;
}

int redis_operion(int adx, int flag, int &err_level, int &print_time)
{
	int errcode = E_SUCCESS;
	char key[100] = "dsp_log_conf";
	redisReply *reply = NULL;

	if (existRedisKey(key) == 0)
	{
		errcode = E_EXIST_GROUPID;
		if (flag != -1)
			errcode = write_Intelligent(key, adx, err_level, print_time);

		goto exit;
	}

	errcode = read_Intelligent(key, flag, adx, err_level, print_time);

exit:
	if (reply != NULL)
		freeReplyObject(reply);

	return errcode;
}

int redis_connecting()
{
	char *sz_redis_ip = NULL;
	int errcode = E_SUCCESS;

	sz_redis_ip = KS_UNICODE_to_UTF8(redis_ip);

	struct timeval timeout = {1, 500000}; // 1.5 seconds
	ctx = redisConnectWithTimeout(sz_redis_ip, redis_port, timeout);
	if ((ctx == NULL) || (ctx->err != 0))
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
		redisFree(ctx);

	return errcode;
}