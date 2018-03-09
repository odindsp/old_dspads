#include "common.h"

off_t get_filesize(char *filename)
{
	struct stat statbuf;
	stat(filename, &statbuf);

	return statbuf.st_size;
}

int gettime(NOW_TIME *now, bool flag)
{
	struct tm tm = { 0 };
	time_t time_temp;
	time(&time_temp);
	localtime_r(&time_temp, &tm);

	if (!flag)
		now->year = tm.tm_year + 1900;
	else
		now->year = tm.tm_year + 1900 - 2000;

	now->mon = tm.tm_mon + 1;
	now->day = tm.tm_mday;
	now->hour = tm.tm_hour;
	now->min = tm.tm_min;
	now->sec = tm.tm_sec;
}

int exist_file(char *filename)
{
	int fd = 0;
	char *temp = (char *)"[default]";
	if (filename == NULL)
		return -1;

	if (access(filename, R_OK) == -1)
	{
		fd = open(filename, O_RDWR | O_CREAT, 0666);
		write(fd, temp, strlen(temp));
	}

	if (fd != 0)
		close(fd);

	return 0;
}
