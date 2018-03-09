#ifndef CONFOPERATION_H_
#define CONFOPERATION_H_

#include <string>

using namespace std;

int GetPrivateProfileInt(char *profile, char *section, char *key);
char *GetPrivateProfileString(char *profile,  char *section, char *key);
int  SetPrivateProfileInt(char *profile, char *section, char *key, int value);
int SetPrivateProfileString(char *profile,  char *section, char *key, char *value);
#endif /* READINIG_H_ */
