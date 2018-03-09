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

char *MD5_file(char *path, int md5_len)
{
	MD5_CTX ctx;
	int len = 0;
	char *file_md5 = NULL;
	unsigned char buffer[1024] = { 0 };
	unsigned char digest[16] = { 0 };

	MD5_Init(&ctx);
	FILE *pFile = fopen(path, "rb");

	while ((len = fread(buffer, 1, 1024, pFile)) > 0)
		MD5_Update(&ctx, buffer, len);

	MD5_Final(digest, &ctx);

	file_md5 = (char *)malloc((md5_len + 1) * sizeof(char));
	if (file_md5 == NULL)
		return NULL;

	memset(file_md5, 0, (md5_len + 1));

	if (md5_len == 16)
	{
		for (int i = 4; i < 12; i++)
			sprintf(&file_md5[(i - 4) * 2], "%02x", digest[i]);
	}
	else if (md5_len == 32)
	{
		for (int i = 0; i < 16; i++)
			sprintf(&file_md5[i * 2], "%02x", digest[i]);
	}
	else
	{
		fclose(pFile);
		free(file_md5);
		return NULL;
	}

	fclose(pFile);
	return file_md5;
}

