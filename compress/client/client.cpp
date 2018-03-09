
/* File Name: client.c */
#include <signal.h>
#include "client.h"
#include "../common/selfsemphore.h"
#include <queue>
#define  CLIENT_CONF             "client_16.conf"
#define  THREAD_COUNT 5

using namespace std;
pthread_mutex_t g_mutex_sendflag, g_mutex_map, g_mutex_queue;
map <string, string> msetflag;

char *log_path = NULL, *source_path = NULL, *server_addr = NULL, *prefix = NULL, *gzprefix = NULL;
queue<FILEINFO> gzip_filename;
string send_path = "";
FILE *log_fd = NULL;
int server_port = 0;
sem_t semid;
pthread_t *id = NULL;
bool run_flag = true;


int create_dir(char *dir)
{
	DIR *send_dir = NULL;

	if ((send_dir = opendir(dir)) == NULL)
	{
		int ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
		if (ret != 0)
			return -1;
	}
	else
	{
	   closedir(send_dir);
	}
	return 0;
}

void *gzip_files(void *args)
{
	char send_dir[MIDLENGTH] = { 0 }, source_dir[MIDLENGTH] = { 0 }, ini_file[MIDLENGTH] = { 0 }, file_time[MIDLENGTH] = { 0 };
	struct dirent *ptr;
	int read_len = 0;
	DIR *dir = NULL;
	NOW_TIME now;
	char hostname[MIDLENGTH] = {0};
	gethostname(hostname,sizeof(hostname));
	gettime(&now, false);
	int pre_day = now.day;
	int pre_mon = now.mon;
	int pre_year = now.year;

	while (run_flag)
	{
		gettime(&now, false);
		int trueday = now.day;
		int truemon = now.mon;
		int trueyear = now.year;

		if (now.day != pre_day)
		{
		   trueday = pre_day;
		   pre_day = now.day;

		   truemon = pre_mon;
		   pre_mon = now.mon;

		   trueyear = pre_year;
		   pre_year = now.year;
		}
		sprintf(file_time, "%d%02d%02d", trueyear, truemon, trueday);
		sprintf(source_dir, "%s/%s_%s/", source_path, prefix, file_time);
		sprintf(send_dir, "%s/%s_%s/", send_path.c_str(), gzprefix, file_time);
		sprintf(ini_file, "%s/client_%s.ini", log_path, file_time);
		exist_file(ini_file);
		create_dir(send_dir);

		dir = opendir(source_dir);
		if (dir == NULL)
		{
			write_log(log_fd, "warning:opendir failed, %s,in %d,at %s\n", source_dir, __LINE__, __FUNCTION__);
			goto exit;
		}

		while ((ptr = readdir(dir)) != NULL)
		{
			char gzfilename[MAXLENGTH] = { 0 }, filename[MAXLENGTH] = { 0 }, buffer[MAXLENGTH] = { 0 }, sourcefile[MAXLENGTH] = {0};
			FILE *fp = NULL;
			gzFile gzfp = 0;
			int gzflag = 0;	
			FILEINFO finfo;

			if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
				continue;
			
			//comresss
			if(strstr(ptr->d_name, "log") != NULL)
			{
			    sprintf(sourcefile, "%s%s.gz", ptr->d_name, hostname);
				
				pthread_mutex_lock(&g_mutex_sendflag);
				gzflag = GetPrivateProfileInt(ini_file, (char *)"default", sourcefile);
				pthread_mutex_unlock(&g_mutex_sendflag);

				sprintf(gzfilename, "%s%s%s.gz", send_dir, ptr->d_name, hostname);
				sprintf(filename, "%s%s", source_dir, ptr->d_name);
				
				finfo.filename = gzfilename;
				finfo.inifile = ini_file;
				finfo.filetime = file_time;

				if (gzflag == GZ_UNSENT)
				{
					fp = fopen(filename, "rb");
					gzfp = gzopen(gzfilename, "wb");
					if ((fp == NULL) || (gzfp == 0))
						continue;

					while ((read_len = fread(buffer, sizeof(char), MAXLENGTH, fp)) > 0)
						gzwrite(gzfp, buffer, read_len);
					
					pthread_mutex_lock(&g_mutex_sendflag);
					SetPrivateProfileInt(ini_file, (char *)"default", sourcefile, GZ_GZ);
					pthread_mutex_unlock(&g_mutex_sendflag);

					fclose(fp);
					gzclose(gzfp);
					pthread_mutex_lock(&g_mutex_queue);
					gzip_filename.push(finfo);
					pthread_mutex_unlock(&g_mutex_queue);
					semaphore_release(semid);
				}
				else if(gzflag == GZ_GZ || gzflag == GZ_INTERRUPT)
				{
					//insert to send queue
					pthread_mutex_lock(&g_mutex_queue);
					gzip_filename.push(finfo);
					pthread_mutex_unlock(&g_mutex_queue);
					semaphore_release(semid);
				}
			}
		}

		closedir(dir);
exit:
		sleep(WAITTIME);
	}
}

void *send_files(void *args)
{
	while(semaphore_wait(semid) && run_flag)
	{
		cout<<"send files"<<endl;
		pthread_mutex_lock(&g_mutex_sendflag);
		pthread_mutex_lock(&g_mutex_queue);
		FILEINFO finfo = gzip_filename.front();
		gzip_filename.pop();
		pthread_mutex_unlock(&g_mutex_queue);
		write_cmd("finfo:%s\n", finfo.filename.c_str());
		SetPrivateProfileInt((char *)finfo.inifile.c_str(), (char *)"default", basename((char *)finfo.filename.c_str()), GZ_SENDING);
		pthread_mutex_unlock(&g_mutex_sendflag);

		int sockfd = -1, npos = 0, timeout = 3000, offset = 0, read_len = 0, filesizes = 0;
		char buffer[MAXLENGTH] = { 0 }, sendline[MAXLENGTH] = { 0 };
		struct sockaddr_in	servaddr;
		FILE *fp = NULL;
		char *file_md5_value = NULL;

retry:
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			write_log(log_fd, "error:create socket error: %s(errno: %d),in %d,at %s\n", 
				strerror(errno), errno, __LINE__, __FUNCTION__);
			goto retry;
		}

		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(server_port);
		if (inet_pton(AF_INET, server_addr, &servaddr.sin_addr) <= 0)
		{
			write_log(log_fd, "error:inet_pton error: %s(errno: %d),in %d,at %s\n", 
				strerror(errno), errno, __LINE__, __FUNCTION__);
			goto retry;
		}

		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));

		if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		{
			write_log(log_fd, "error:connect error: %s(errno: %d),in %d,at %s\n", 
				strerror(errno), errno, __LINE__, __FUNCTION__);
			goto retry;
		}

		filesizes = get_filesize((char *)finfo.filename.c_str());
		file_md5_value = MD5_file((char *)finfo.filename.c_str(), MD5_MODE);
		if (file_md5_value == NULL)
		{
			write_log(log_fd, "md5 error");
			goto exit;
		}
		sprintf(sendline, "%s %s %ld %s", basename((char *)finfo.filename.c_str()), file_md5_value, filesizes, (char *)finfo.filetime.c_str());
		write_log(log_fd, "filename:%s,filemd5:%s,filesizes:%d\n", basename((char *)finfo.filename.c_str()), file_md5_value, filesizes);
		IFREE(file_md5_value);
		send(sockfd, sendline, sizeof(sendline), 0);
		recv(sockfd, buffer, MINILENGTH, 0);
		offset = atoi(buffer);
		if (offset == filesizes)
			goto exit;

		pthread_mutex_lock(&g_mutex_map);
		msetflag.insert(make_pair(basename((char *)finfo.filename.c_str()), finfo.inifile));
		pthread_mutex_unlock(&g_mutex_map);

		fp = fopen(finfo.filename.c_str(), "rb");
		if (fp == NULL)
		{
			write_log(log_fd, "error:fopen %s failed,in %d,at %s!\n", finfo.filename.c_str(), __LINE__, __FUNCTION__);
			goto exit;
		}

		fseek(fp, offset, SEEK_SET);

		while (1)
		{
			read_len = fread(buffer, sizeof(char), MAXLENGTH, fp);
			if (read_len > 0)
			{
				npos += read_len;
				if (send(sockfd, buffer, read_len, 0) < 0)  // socket error
				{
					if (fp != NULL)
					{
						fclose(fp);
						fp = NULL;
					}

					if (sockfd !=  -1)
					{
						close(sockfd);
						sockfd = -1;
					}
					goto retry;
				}
			}
			else
			{
				pthread_mutex_lock(&g_mutex_map);
				map <string, string>::iterator it = msetflag.find(basename((char *)finfo.filename.c_str()));
				if (it != msetflag.end())
					msetflag.erase(it);
				pthread_mutex_unlock(&g_mutex_map);

				pthread_mutex_lock(&g_mutex_sendflag);
				SetPrivateProfileInt((char *)finfo.inifile.c_str(), (char *)"default", basename((char *)finfo.filename.c_str()), GZ_SENT);
				pthread_mutex_unlock(&g_mutex_sendflag);

				break;
			}
			bzero(buffer, MAXLENGTH);
		}
		bzero(buffer, MAXLENGTH);
		recv(sockfd, buffer, SOCKET_FLAG, 0);   // 0 is peer close
		if (strcmp(buffer, "true"))
		{
			write_log(log_fd, "error:retry to send data, %s,in %d,at %s\n", finfo.filename.c_str(), __LINE__, __FUNCTION__);
			if (fp != NULL)
			{
				fclose(fp);
				fp = NULL;
			}

			if (sockfd !=  -1)
			{
				close(sockfd);
				sockfd = -1;
			}
			goto retry;
		}

exit:

		if (fp != NULL)
		{
			fclose(fp);
			fp = NULL;
		}

		if (sockfd !=  -1)
		{
			close(sockfd);
			sockfd = -1;
		}
	}

	cout<<"send file exit"<<endl;
}
void free_space()
{
	if (log_fd != 0)
		close_file(log_fd);

	IFREE(prefix);
	IFREE(gzprefix);
	IFREE(log_path);
	IFREE(server_addr);
	IFREE(source_path);
}

void signal_handler(int signalNo)
{
	run_flag = false;
	for(int i = 0; i < THREAD_COUNT; ++i)
	{
		semaphore_release(semid);
	}
}

void end_deal()
{
	map <string, string>::iterator it;
	pthread_mutex_lock(&g_mutex_map);
	for (it = msetflag.begin(); it != msetflag.end(); ++it)
	{
		pthread_mutex_lock(&g_mutex_sendflag);
		SetPrivateProfileInt((char *)it->second.c_str(), (char *)"default", (char *)it->first.c_str(), GZ_INTERRUPT);
		pthread_mutex_unlock(&g_mutex_sendflag);

		write_log(log_fd, "error:interrupt,the filename:%s,in %d,at %s\n", it->first.c_str(), __LINE__, __FUNCTION__);
	}
	pthread_mutex_unlock(&g_mutex_map);
}


int main(int argc, char** argv)
{
	pthread_t gzip_id;
	string client_config = "";
	bool flag;
	client_config = /*string(CONFIG_PATH) +*/ string(CLIENT_CONF);
	log_path = GetPrivateProfileString((char *)client_config.c_str(), (char *)"log", (char *)"log_path");
	source_path = GetPrivateProfileString((char *)client_config.c_str(), (char *)"default", (char *)"source_path");//not me created.
	prefix = GetPrivateProfileString((char *)client_config.c_str(), (char *)"default", (char *)"prefix");
	gzprefix = GetPrivateProfileString((char *)client_config.c_str(), (char *)"default", (char *)"gzprefix");
	server_addr = GetPrivateProfileString((char *)client_config.c_str(), (char *)"server", (char *)"server_addr");
	server_port = GetPrivateProfileInt((char *)client_config.c_str(), (char *)"server", (char *)"server_port");
	if ((log_path == NULL) || (source_path == NULL) || (server_addr == NULL) || (server_port == 0))
	{
		write_cmd("error:bad configure file,send_path:%s,log_path:%s,source:%s,server_addr:%s,server_port:%d,in %d,at %s\n", 
			send_path.c_str(), log_path, source_path, server_addr, server_port, __LINE__, __FUNCTION__);
		goto exit;
	}

	// create log system
	log_fd = log_file(log_path, (char *)"client", MINTUE_MODE);
	if (log_fd == NULL)
	{
		write_cmd("can't create log_fd, in %d, at %s\n", __LINE__, __FUNCTION__);
		goto exit;
	}
		
	/* init mutex and signal */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGKILL, signal_handler);
	pthread_mutex_init(&g_mutex_sendflag, 0);
	pthread_mutex_init(&g_mutex_map, 0);
	pthread_mutex_init(&g_mutex_queue, 0);
	send_path = string(source_path);
	if(!createsemaphore(semid))
	{
		goto exit;
	}
	id = (pthread_t *)calloc(THREAD_COUNT, sizeof(pthread_t));
	if (id == NULL)
	{
		va_cout("id malloc failed,at:%s,in:%d", __FUNCTION__, __LINE__);
		goto exit;
	}
	
	pthread_create(&gzip_id, NULL, gzip_files, NULL);
	

	for(int i = 0; i < THREAD_COUNT; ++i)
	{
		pthread_create(&id[i], NULL, send_files, NULL);
	}
	pthread_join(gzip_id, NULL);
	for (uint8_t i = 0 ; i < THREAD_COUNT; ++i)
	{
		pthread_join(id[i], NULL);
	}

	end_deal();

exit:
	free_space();
	pthread_mutex_destroy(&g_mutex_sendflag);
	pthread_mutex_destroy(&g_mutex_map);
	pthread_mutex_destroy(&g_mutex_queue);
	IFREE(id);
	return 0;
}
