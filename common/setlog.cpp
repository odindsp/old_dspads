#include <sys/time.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include "util.h"
#include "setlog.h"
#include "tcp.h"
#include "selfsemphore.h"
#include "confoperation.h"
using namespace std;

#define LOG "log"

static string log_level_info[LOGMAX + 1] = {"NON", "INF", "WRN", "ERR", "DBG"};

pthread_t thread_id;

//typedef struct Node
//{
//	uint8_t level;
//	string content;
//}NODE;

struct user_log
{
	string dir;
	char folder[128];
	char  section[64];
	char filename[MID_LENGTH];
	string fileprefix;
	char conffile[64];
	//int subindex;
	int pre_tm_day;
	int pre_tm_hour;
	FILE *fd;
	queue<string> log;
	uint8_t level;
	pthread_mutex_t mutex_log;
	bool is_textdata;
	uint64_t maxsize;
	bool is_print_queue_size;
	bool hdfslog;
	//int semid;
	bool *run_flag;
};
typedef struct user_log USERLOG;

/*
void *threadLog(void *arg)
{
	pthread_detach(pthread_self());
	char logprefix[MID_LENGTH];
	USERLOG *userlog = (USERLOG *)arg;
	ofstream *fout;

	static int  count = 0;

	while (semaphore_wait(userlog->semid))
	{
		printf("recv write notice\n");
		pthread_mutex_lock(&userlog->mutex_log);

		struct Node *tmpnode = userlog->log.front();
		
		if (userlog->tm_day != userlog->pre_tm_day)
		{
			userlog->pre_tm_day = userlog->tm_day;
	
			sprintf(userlog->folder, "%s/%s_%04d%02d%02d", userlog->dir.c_str(),
				userlog->fileprefix.c_str(), userlog->tm_year, userlog->tm_mon,
				userlog->tm_day);
			//printf("%s\n", userlog->folder);
			
			int ret = mkdir(userlog->folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (ret == 0)
			{
				userlog->subindex = 0;
				//将修改写回配置文件
				SetPrivateProfileInt(userlog->conffile, userlog->section, "index", userlog->subindex);
			}
			
			//printf("mkdir failed %d\n", errno);
			//perror("perror");
			userlog->pre_tm_day = userlog->tm_day;
			
			if(userlog->fd != NULL)
			{
				fclose(userlog->fd);
				userlog->fd = NULL;
			}
		}
		if(userlog->fd  == NULL)
		{
			char filename[MID_LENGTH] = "";
			sprintf(filename, "%s/%s%04d%02d%02d_%d", userlog->folder, userlog->fileprefix.c_str(),userlog->tm_year, userlog->tm_mon,
				userlog->tm_day, userlog->subindex);
			//printf("%s\n", filename);
			userlog->fd = fopen(filename, "a+");
			if(userlog->fd == NULL)
				printf("open file failed\n");
		}

		//int ret = fputs(tmpnode->content.c_str(), userlog->fd );
		int len = tmpnode->content.length();
		//printf("len is %d\n", len);
		int ret = fwrite(tmpnode->content.c_str(),  1, len, userlog->fd);
		//printf("write success ret is %d\n", ret);

		//int ret = fprintf(userlog->fd, "%s\n", tmpnode->content.c_str());
		//printf("fprintf ret is %d\n", ret);
		fflush(userlog->fd);
		//printf("content is %s", tmpnode->content.c_str());		
		//count += tmpnode.length();
		delete tmpnode;
		count += ret;
		if(count >= userlog->maxsize)
		{
			userlog->subindex ++;
			//将修改写回配置文件
			SetPrivateProfileInt(userlog->conffile, userlog->section, "index", userlog->subindex);
			char filename[MID_LENGTH] = "";
			sprintf(filename, "%s/%s%04d%02d%02d_%d", userlog->folder,userlog->fileprefix.c_str(),userlog->tm_year, userlog->tm_mon,
				userlog->tm_day, userlog->subindex);
			fclose(userlog->fd);
			userlog->fd = fopen(filename, "a+");
			count = 0;
		}	

		userlog->log.pop();
		pthread_mutex_unlock(&userlog->mutex_log);
	}
	return NULL;
}*/

void *threadLog(void *arg)
{
	char logprefix[MID_LENGTH];
	USERLOG *userlog = (USERLOG *)arg;
	uint64_t  count = 0;
	int index = 0;
	int ret;

	//pthread_detach(pthread_self());

	while ((*userlog->run_flag))
	{
		time_t timep;
		struct tm tm = {0};

		time(&timep);
		localtime_r(&timep, &tm);
		if (tm.tm_mday != userlog->pre_tm_day || tm.tm_hour != userlog->pre_tm_hour)
		{
			if(tm.tm_mday != userlog->pre_tm_day)
			{
				userlog->pre_tm_day = tm.tm_mday;
				userlog->pre_tm_hour = tm.tm_hour;
				sprintf(userlog->folder, "%s/%s_%04d%02d%02d", userlog->dir.c_str(),
					userlog->fileprefix.c_str(), tm.tm_year + 1900, tm.tm_mon + 1,
					tm.tm_mday);
				

				ret = mkdir(userlog->folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				if (ret == 0)
				{
					//userlog->subindex = 0;
					//将修改写回配置文件
					//SetPrivateProfileInt(userlog->conffile, userlog->section, "index", userlog->subindex);
					va_cout("create folder %s success", userlog->folder);
				}
			}
			else
			{
				userlog->pre_tm_hour = tm.tm_hour;
			}
			
			if(userlog->fd != NULL)
			{
				fclose(userlog->fd);
				if(userlog->hdfslog)
				{
					//rename
					char newname[MID_LENGTH] = "";
					sprintf(newname, "%s.log", userlog->filename);
					int ret = rename(userlog->filename, newname);
					if(ret != 0)
					{
						syslog(LOG_INFO, "odin %s rename error %s", userlog->section, strerror(errno));
					}
				}
				userlog->fd = NULL;
			}
		}

		pthread_mutex_lock(&userlog->mutex_log);
		if (userlog->log.size() > 0)
		{
			if( userlog->is_print_queue_size)
			{
				++index;
				if(index >100)
				{
					syslog(LOG_INFO, "odin %s size %d", userlog->section, userlog->log.size());
					index = 0;
				}
			}
			string logdata = "";
			for (int i = 0; i < 16; i++)
			{
				logdata += userlog->log.front();
				userlog->log.pop();
				if (userlog->log.empty())
					break;
			}
			pthread_mutex_unlock(&userlog->mutex_log);

			if(userlog->fd  == NULL)
			{
				struct timespec ts;
				time_t timep;
				struct tm tm = {0};

				time(&timep);
				localtime_r(&timep, &tm);
				clock_gettime(CLOCK_REALTIME, &ts);
				if(userlog->hdfslog)
				{
					sprintf(userlog->filename, "%s/%s_%04d%02d%02d%02d%02d%02d%03ld", userlog->folder, userlog->fileprefix.c_str(), tm.tm_year + 1900 , tm.tm_mon + 1,
						tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}
				else
				{
					sprintf(userlog->filename, "%s/%s_%02d%02d%02d%03ld.log", userlog->folder, userlog->fileprefix.c_str(), tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}
				//va_cout("%s\n", filename);
				userlog->fd = fopen(userlog->filename, "a+");
				if(userlog->fd == NULL)
					va_cout("open file %s failed", userlog->filename);
				count = 0;
			}

			count += logdata.size();
			if(count >= userlog->maxsize)
			{
				//关闭旧的文件描述符
				fclose(userlog->fd);
				if(userlog->hdfslog)
				{//rename
					char newname[MID_LENGTH] = "";
					sprintf(newname, "%s.log", userlog->filename);
					ret = rename(userlog->filename, newname);
					if(ret != 0)
					{
						syslog(LOG_INFO, "odin %s rename error %s", userlog->section, strerror(errno));
					}
				}

				struct timespec ts;
				time_t timep;
				struct tm tm = {0};

				time(&timep);
				localtime_r(&timep, &tm);
				clock_gettime(CLOCK_REALTIME, &ts);
				if(userlog->hdfslog)
				{
					sprintf(userlog->filename, "%s/%s_%04d%02d%02d%02d%02d%02d%03ld", userlog->folder, userlog->fileprefix.c_str(), tm.tm_year + 1900 , tm.tm_mon + 1,
						tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}
				else
				{
					sprintf(userlog->filename, "%s/%s_%02d%02d%02d%03ld.log", userlog->folder, userlog->fileprefix.c_str(), tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}
				//va_cout("%s\n", filename);
				userlog->fd = fopen(userlog->filename, "a+");
				if(userlog->fd == NULL)
					va_cout("open file %s failed", userlog->filename);

				count = logdata.size();
			}	
			ret = fwrite(logdata.c_str(), logdata.size(), 1, userlog->fd);
			if(ret != 1)
			{
				syslog(LOG_INFO, "odin %s write error %s", userlog->section, strerror(errno));
			}
			fflush(userlog->fd);

		}
		else
		{
			pthread_mutex_unlock(&userlog->mutex_log);
			usleep(100000);
		}
	}
	pthread_mutex_lock(&userlog->mutex_log);
	if (userlog->log.size() > 0)
	{
		string logdata = "";
		for (int i = 0; i < userlog->log.size(); ++i)
		{
			logdata += userlog->log.front();
			userlog->log.pop();
		}
		if (userlog->fd != NULL)
		{
			fwrite(logdata.c_str(), logdata.size(), 1, userlog->fd);
			fflush(userlog->fd);
		}
	}
	pthread_mutex_unlock(&userlog->mutex_log);
	cout<<"threadLog exit"<<endl;
	syslog(LOG_INFO, "dsp threadLog exit");
	return NULL;
}

uint64_t openLog(const char * conffile, const char *section, bool is_textdata, bool *run_flag, vector<pthread_t> &v_thread_id, bool is_print_queue_size)
{
	USERLOG *userlog;
	//pthread_t thread;
	if(access(conffile, F_OK) != 0)
		return 0;

	userlog = new USERLOG;
	if (userlog == NULL)
	{
		va_cout("new USERLOG fail");
		return 0;
	}
	char *strpre = GetPrivateProfileString((char *)(string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE)).c_str(), "log", "gpath");
	if(strpre == NULL)
		return 0;
	char *strini = GetPrivateProfileString(conffile, section, "path");
	if(strini == NULL)
		return 0;

	if(strcmp(section, "loghdfs"))
	{
		userlog->hdfslog = false;
	}
	else
	{
		userlog->hdfslog = true;
	}

	userlog->dir = string(strpre) + "/" + string(strini);
	free(strpre);
	free(strini);
	char command[128] = "";
	sprintf(command, "mkdir -p %s", userlog->dir.c_str());
	cout << "logdir :" <<userlog->dir <<endl;
	system(command);//递归创建文件夹
	/*int ret = mkdir(userlog->dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (ret != 0 && errno != EEXIST)
	{
		va_cout("create dir %s failed, errno is %d and strerror %s", userlog->dir.c_str(), errno, strerror(errno));
		return 0;
	}*/

	strcpy(userlog->section, section);
	strini = GetPrivateProfileString(conffile, section, "file");
	if(strini == NULL)
		return 0;
	userlog->fileprefix = strini;
	free(strini);

	strcpy(userlog->conffile, conffile);

	userlog->level =  GetPrivateProfileInt(conffile, section, "level");
	if(userlog->level <= 0)
		userlog->level = LOGINFO;
	else if(userlog->level > LOGDEBUG)
		userlog->level = LOGDEBUG;
	uint64_t ssize = GetPrivateProfileInt(conffile, section, "size");
	userlog->maxsize = ssize * 1024 *1024;
	if(userlog->maxsize == 0)
		userlog->maxsize = 1024*1024;
	//userlog->subindex = GetPrivateProfileInt(conffile, section, "index");
	va_cout("level is %d, maxsize is %ld\n", userlog->level, userlog->maxsize);

	/*userlog->semid =  createsemaphore();
	set_semvalue(userlog->semid);*/
	userlog->fd = NULL;
	//userlog->folder = "";
	userlog->pre_tm_day = 0;
	userlog->pre_tm_hour = 0;

	pthread_mutex_init(&userlog->mutex_log, 0);
	userlog->is_textdata = is_textdata;
	userlog->is_print_queue_size = is_print_queue_size;
	userlog->run_flag = run_flag;
	pthread_create(&thread_id, NULL, threadLog, userlog);
	v_thread_id.push_back(thread_id);
	return (uint64_t)(userlog);
}

int closeLog(const uint64_t logid)
{
	USERLOG *userlog = (USERLOG *)logid;

	pthread_mutex_destroy(&userlog->mutex_log);
	if (userlog->fd != NULL)
	{
		fclose(userlog->fd);
		if(userlog->hdfslog)
		{//rename
			char newname[MID_LENGTH] = "";
			sprintf(newname, "%s.log", userlog->filename);
			int ret = rename(userlog->filename, newname);
			if(ret == 0)
			{
				va_cout("rename filename :%s success", userlog->filename);
			}
		}
	}
	delete userlog;
}

int writeLog(const uint64_t logid, uint8_t log_level, const char *fmt, ...)
{
	USERLOG *userlog = (USERLOG *)logid;
	if(log_level < LOGMIN || log_level > LOGMAX || log_level < userlog->level)
		return 0;

	va_list argptr;
	char *writecontent = NULL;

	int n, size = 256;
	char *np;
	va_list ap;

	if ((writecontent = (char *)malloc(size)) == NULL)
		return 1;

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(writecontent, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = (char *)realloc (writecontent, size)) == NULL) {
			free(writecontent);
			writecontent = NULL;
			break;
		} else {
			writecontent = np;
		}
	}
	if(writecontent == NULL)
		return 1;

	string data = writecontent;
	writeLog(logid, log_level, data);
	free(writecontent);
	return 0;
}

int writeLog(const uint64_t logid, uint8_t log_level, string data)
{
	USERLOG *userlog = (USERLOG *)logid;
	if(log_level < LOGMIN || log_level > LOGMAX || log_level < userlog->level)
		return 0;

	struct timeval timeofday;
	//struct tm *tm;
	//NODE *tmpnode;
	struct tm tm = { 0 };
	time_t timep;

	time(&timep);
	localtime_r(&timep, &tm);
	gettimeofday(&timeofday, NULL);

	//tmpnode = new NODE;
	//tmpnode->level = log_level;

	if (userlog->is_textdata)
	{
		char logprefix[MID_LENGTH];
		
		sprintf(logprefix, "%04d%02d%02d,%02d:%02d:%02d.%03ld", tm.tm_year + 1900,
			tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
			timeofday.tv_usec / 1000);
		pthread_mutex_lock(&userlog->mutex_log);
		userlog->log.push(string(logprefix, strlen(logprefix))  + ','+ log_level_info[log_level] + ',' + data + '\n');
		pthread_mutex_unlock(&userlog->mutex_log);
		//tmpnode->content = string(logprefix, strlen(logprefix))  + '\t'+ log_level_info[log_level] + '\t' + data + "\n";
	}
	else
	{
		uint32_t length, hllength;
		uint64_t cm, hlcm;

		length = data.size();
		hllength = htonl(length);
		cm = timeofday.tv_sec * 1000 + timeofday.tv_usec / 1000;
		hlcm = htonl64(cm);

		//printf("cm is %lu and hlcm is %lu\n", cm, hlcm);
		pthread_mutex_lock(&userlog->mutex_log);
		userlog->log.push(string((char *)&hllength, sizeof(uint32_t))
			+ string((char *)&hlcm, sizeof(uint64_t)) + data + "\n");
		pthread_mutex_unlock(&userlog->mutex_log);
		/*tmpnode->content = string((char *)&hllength, sizeof(uint32_t))
			+ string((char *)&hlcm, sizeof(uint64_t))  + '\t'+ log_level_info[log_level] + '\t' + data + "\n";*/

	}
	return 0;
}

int writeLog(int logid, string data)
{
	USERLOG *userlog = (USERLOG *)logid;
	struct timeval timeofday;
	uint64_t cm;
	char strcm[16]= "";

	gettimeofday(&timeofday, NULL);

	cm = timeofday.tv_sec * 1000 + timeofday.tv_usec / 1000;

	snprintf(strcm, 16, "%llu", cm);
	pthread_mutex_lock(&userlog->mutex_log);
	userlog->log.push(string(strcm)+ '\t' + data + '\n');
	pthread_mutex_unlock(&userlog->mutex_log);
	return 0;
}

void cflog(IN uint64_t logid, IN uint8_t level, const char *fmt, ...)
{
	USERLOG *userlog = (USERLOG *)logid;
	if(level < LOGMIN || level > LOGMAX || level < userlog->level)
		return;

	int n, size = 256;
	char *writecontent, *np;
	va_list ap;

	if ((writecontent = (char *)malloc(size)) == NULL)
		return;

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(writecontent, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = (char *)realloc (writecontent, size)) == NULL) {
			free(writecontent);
			writecontent = NULL;
			break;
		} else {
			writecontent = np;
		}
	}
	if(writecontent != NULL)
	{
		va_cout("%s", writecontent);
		writeLog(logid, level, string(writecontent, strlen(writecontent)));
		free(writecontent);
	}

}

/*int main(int argc, char *argv[])
{
	char data[MID_LENGTH] = "1234567890";
	int length = strlen(data);

	uint64_t logid = openLog("dsp_02", "dsp_02", "log.ini", true);
	//uint64_t logid = openLog("dsp_02", "dsp_02","log.ini", false);

	//uint64_t logid2 = openLog("dsp_01", "dsp_01","log.ini", true);
	if(logid)
		printf("open success\n");
	for (int i = 0; i < 3; i++)
	{
		writeLog(logid, LOGINFO, string(data, length));
		writeLog(logid, LOGINFO, "hello %s", "world");
		//writeLog(logid2, LOGINFO, string(data, length));
	}
	pthread_join(thread_id, NULL);	
	closeLog(logid);
	//closeLog(logid2);

	return 0;
}*/
