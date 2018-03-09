#include "writelog.h"
#include "common.h"

FILE *log_file(char *path, char *prefix, int mode)
{
	char clog_path[MIDLENGTH] = "", file_path[MIDLENGTH] = "", filetime[32] = "";
	struct tm tm_time;
	time_t t_log;
	string log_time = "";
	FILE *fd = NULL;

	sprintf(clog_path, "mkdir -p %s", path);
	cout << "clog_path:" << clog_path << endl;
	system(clog_path);

	t_log = time(NULL);
	localtime_r(&t_log, &tm_time);
	strftime(filetime, sizeof(filetime), "%Y%m%d%H%M%S", &tm_time);

	switch (mode)
	{
	case MINTUE_MODE:
		log_time.assign(filetime, 0, 12);
		break;
	case HOUR_MODE:
		log_time.assign(filetime, 0, 12);
		break;
	case DAY_MODE:
		log_time.assign(filetime, 0, 8);
		break;
	case MONTH_MODE:
		log_time.assign(filetime, 0, 6);
		break;
	default:
		log_time.assign(filetime, 0, 12);
		break;
	}

	sprintf(file_path, "%s/%s_", path, prefix);
	strcat(file_path, log_time.c_str());
	strcat(file_path, ".log");
	cout << "file_path:" << file_path << endl;

	fd = fopen(file_path, "wb");
	assert(fd != NULL);
	return fd;
}

void write_cmd(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

int write_log(FILE *fd, const char *msg, ...)
{
	char final[2048] = { 0 }, content[1024] = { 0 };
	va_list vl_list;
	va_start(vl_list, msg);
	vsprintf(content, msg, vl_list);
	va_end(vl_list);

	struct tm tm = { 0 };
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	strftime(final, sizeof(final), "%Y-%m-%d %H:%M:%S,", &tm);
	strncat(final, content, strlen(content));
	assert((msg != NULL) && (fd != NULL));
	assert(fwrite(final, sizeof(char), strlen(final), fd) == strlen(final));
	fflush(fd);

	return 0;
}

void close_file(FILE *fd)
{
	if (fd != NULL)
		fclose(fd);
}
