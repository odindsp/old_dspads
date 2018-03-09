#ifndef CONFOPERATION_H_
#define CONFOPERATION_H_

#include <string>

using namespace std;

int GetPrivateProfileInt(const char *profile, const char *section, const char *key);
char *GetPrivateProfileString(const char *profile, const char *section, const char *key);
int  SetPrivateProfileInt(const char *profile, const char *section, const char *key, int value);
int SetPrivateProfileString(const char *profile, const char *section, const char *key, char *value);
#endif /* READINIG_H_ */
