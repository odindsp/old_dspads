#ifndef _CLIENT_H_
#define  _CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/stat.h>    
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "zlib.h"
#include "../common/common.h"
#include "../common/writelog.h"
#include "../common/confoperation.h"

using namespace std;

#define		GZ_UNSENT		0
#define     GZ_GZ			1
#define		GZ_SENDING		2
#define		GZ_INTERRUPT	3
#define		GZ_SENT			4

#endif
