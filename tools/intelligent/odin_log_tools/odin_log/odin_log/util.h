#ifndef UTIL_H
#define UTIL_H
#include "json.h"
#include "errcode.h"
#include <vector>
#include <string>

#define		FILENAME_INI		"config.ini"
#define		SECTION_IP			"IP"
#define		SECTION_PORT		"PORT"
#define		SECTION_GROUPID		"GROUPID"

struct intelligent_info
{
	int adx;
	int flag;
	int interval;
	int minprice;
	int maxprice;
};
typedef struct intelligent_info INTELLIGENT_INFO;

int redis_connecting();
int redis_deconnect();
WCHAR* KS_UTF8_to_UNICODE(const char *szUTF8);
char* KS_UNICODE_to_UTF8(const LPCWSTR wcsString);
int redis_operion(int adx, int flag, int &err_level, int &print_time);
void json_insert_int(json_t *entry, const char *label, int value);
void json_insert_string(json_t *entry, const char *label, const char * value);
DWORD IniReadStrByNameA(char *file, char *section, char *key, char *szOut);
BOOL IniWriteStrByNameA(char *file, char *section, char *key, char *str);
BOOL IniWriteStrByNameW(WCHAR *file, WCHAR *section, WCHAR *key, WCHAR *str);
#endif