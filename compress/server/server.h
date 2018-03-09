#ifndef _SERVER_H_
#define  _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>    
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <libgen.h>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include "../common/common.h"
#include "../common/writelog.h"
#include "../common/confoperation.h"
using namespace std;

#define HDFS_PATH_PER_0			0
#define HDFS_PATH_PER_20		20
#define HDFS_PATH_PER_40		40

#endif