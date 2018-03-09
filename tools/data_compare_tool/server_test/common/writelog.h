#ifndef _WRITELOG_H_
#define _WRITELOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string>
#include <iostream>

using namespace std;

#define MINTUE_MODE			1
#define HOUR_MODE			2
#define DAY_MODE			3
#define MONTH_MODE			4

FILE *log_file(char *path, char *prefix, int mode);
int write_log(FILE *, const char *msg, ...);
void write_cmd(const char *fmt, ...);
void close_file(FILE *fd);

#endif
