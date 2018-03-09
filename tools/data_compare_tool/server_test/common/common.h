#ifndef _COMMON_H_
#define  _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>

#define DEFAULT_PORT 		59001

#define MAXLENGTH			1024
#define MIDLENGTH			512
#define MINLENGTH			128
#define MINILENGTH			64

#define MD5_MODE			32
#define SOCKET_FLAG			10

#define WAITTIME			20

#define CONFIG_PATH			"/etc/dspads_odin/"

struct now_time
{
	int year;
	int mon;
	int day;
	int hour;
	int min;
	int sec;
};
typedef struct now_time NOW_TIME;

struct fileinfo
{
	char filename[MAXLENGTH];
	char tempfile[MAXLENGTH];
};
typedef struct fileinfo FILEINFO;

int	gettime(NOW_TIME *now, bool flag);
int	exist_file(char *filename);
off_t get_filesize(char *filename);

#endif
