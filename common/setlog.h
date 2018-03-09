#ifndef SETLOG_H_
#define SETLOG_H_

#include "type.h"

using namespace std;

#define LOGMIN			LOGINFO

#define LOGINFO			1
#define LOGWARNING		2
#define LOGERROR		3
#define LOGDEBUG		4

#define LOGMAX			LOGDEBUG

uint64_t openLog(const char * conffile, const char *section, bool is_textdata, bool *run_flag, vector<pthread_t> &v_thread_id, bool is_print_queue_size = false);
int writeLog(const uint64_t logid, uint8_t log_level, string data);
int writeLog(const uint64_t logid, uint8_t log_level, const char *fmt, ...);
int writeLog(int logid, string data);
int closeLog(const uint64_t logid);
void cflog(IN uint64_t logid, IN uint8_t level, const char *fmt, ...);

#endif /* SETLOG_H_ */
