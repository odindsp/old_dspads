#ifndef UTIL_H
#define UTIL_H
#include "json.h"
#include "errcode.h"
#include <vector>
#include <string>
using namespace std;

#define		FILENAME_INI		"config.ini"
#define		SECTION_INFO		"INFO"
#define		MAX_LENGTH			10000
#define		MID_LENGTH			1000
#define		MIN_LENGTH			100
#define		MINI_LENGTH			20

struct intelligent_info
{
	int adx;
	int flag;
	int interval;
	double minratio;
	double maxratio;
	double valid_time;
};
typedef struct intelligent_info INTELLIGENT_INFO;

struct aver_price
{
	char mapid[MIN_LENGTH];
	double real_price;
	double out_price;
	int finish_time;
	char size[MINI_LENGTH];
};
typedef struct aver_price AVER_PRICE;

int redis_connecting();
int redis_deconnect();
WCHAR* KS_UTF8_to_UNICODE(const char *szUTF8);
char* KS_UNICODE_to_UTF8(const LPCWSTR wcsString);
int redis_operion(TCHAR *groupid, INTELLIGENT_INFO &v_info);
void json_insert_int(json_t *entry, const char *label, int value);
int get_aver_price(char *get_groupid_info, int adx, vector <AVER_PRICE> &averprice, double &left_time);
double get_keys_left_time(char *groupid);
void json_insert_string(json_t *entry, const char *label, const char * value);
DWORD IniReadStrByNameA(char *file, char *section, char *key, char *szOut);
BOOL IniWriteStrByNameA(char *file, char *section, char *key, char *str);
BOOL IniWriteStrByNameW(WCHAR *file, WCHAR *section, WCHAR *key, WCHAR *str);
#endif