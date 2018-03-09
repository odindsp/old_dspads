/* File Name: server.cpp */
#include "server.h"
#include "hdfs.h"

#define		SERVER_CONF			"server_16.conf"
#define     SERVER_COUNT   20

char *hdfs_path = NULL, *hdfs_addr = NULL, *log_path = NULL, *save_path = NULL, *prefix = NULL;
pthread_mutex_t g_mutex_setflag, g_mutex_qwrite_hdfs;
pthread_t *id = NULL;
FILE *log_fd = NULL;
int hdfs_port = 0;
hdfsFS fs = NULL;

queue <string> qwrite_hdfs;

int get_hdfs_path(int now_min)
{
	int hdfs_path_p = HDFS_PATH_PER_0;

	if ((now_min >= HDFS_PATH_PER_0) && (now_min < HDFS_PATH_PER_20))
		hdfs_path_p = HDFS_PATH_PER_0;
	else if ((now_min >= HDFS_PATH_PER_20) && (now_min < HDFS_PATH_PER_40))
		hdfs_path_p = HDFS_PATH_PER_20;
	else
		hdfs_path_p = HDFS_PATH_PER_40;

	return hdfs_path_p;
}

int write_HDFS(string filename)
{
	char writepath[MIDLENGTH] = { 0 }, buffer[MAXLENGTH] = { 0 }, temp1[20] = { 0 };
	int curSize = 0, adx = 0, myear = 0, mmon = 0, mday = 0, mhour = 0, mmin = 0, msec = 0;
	hdfsFile writeFile;
	NOW_TIME nowtime;
	FILE *fp = NULL;

	sscanf(basename((char *)filename.c_str()), "%[^_]_%02d_%04d%02d%02d%02d%02d%05d.", temp1, &adx, &myear, &mmon, &mday, &mhour, &mmin, &msec);
	sprintf(writepath, "%s/%02d-%02d-%02d/%02d%02d/%s", hdfs_path, myear - 2000, mmon, mday, mhour, get_hdfs_path(mmin), basename((char *)filename.c_str()));
	writeFile = hdfsOpenFile(fs, writepath, O_WRONLY | O_CREAT, MAXLENGTH, 0, 0);
	if (!writeFile)
	{
		write_log(log_fd, "error:can't open %s for writing,in %d,at %s\n", writepath, __LINE__, __FUNCTION__);
		goto exit;
	}

	fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
	{
		write_log(log_fd, "error:can't open %s for writing,in %d,at %s\n", filename.c_str(), __LINE__, __FUNCTION__);
		goto exit;
	}

	while ((curSize = fread(buffer, sizeof(char), MAXLENGTH, fp)) > 0)
			hdfsWrite(fs, writeFile, (void *)buffer, curSize);

exit:
	if (fp != NULL)
		fclose(fp);

	hdfsCloseFile(fs, writeFile);
}

void *accept_files(void *args)
{
	int socket_fd = *(int *)args;
	int connect_fd = 0;
	long filesize = 0, ser_recv_size = 0;
	char buffer[MAXLENGTH] = { 0 }, filename[MINILENGTH] = { 0 },
		filemd5[MINILENGTH] = { 0 }, server_recv[MINILENGTH] = { 0 },
		tmpserver[MIDLENGTH] = { 0 }, filesave_path[MIDLENGTH] = { 0 },
		file_time[MIDLENGTH] = { 0 };

	while (1)
	{
		FILE *fp = NULL;
		NOW_TIME now;
		int pos = 0, length = 0;
		bool recv_flag;
		char mk_save_path[MIDLENGTH] = "";
		char *file_md5_value = NULL;

		if ((connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1)
		{
			write_log(log_fd, "error:accept socket error: %s(errno: %d),in %d,at %s\n", 
				strerror(errno), errno, __LINE__, __FUNCTION__);
			break;
		}
		
		recv(connect_fd, buffer, MAXLENGTH, 0);
		sscanf(buffer, "%s %s %ld %s", filename, filemd5, &filesize, file_time);
		gettime(&now, false);
		sprintf(tmpserver, "%s/server_%s.ini", log_path, file_time);
		sprintf(filesave_path, "%s/%s_%s/", save_path, prefix, file_time);
		sprintf(mk_save_path, "mkdir -p %s", filesave_path);
		exist_file(tmpserver);
		system(mk_save_path);
//		write_log(log_fd, "recv buffer:%s", buffer);

		pthread_mutex_lock(&g_mutex_setflag);
		ser_recv_size = GetPrivateProfileInt(tmpserver, (char *)"default", filename);
		pthread_mutex_unlock(&g_mutex_setflag);
		
		sprintf(server_recv, "%ld", ser_recv_size);
		send(connect_fd, server_recv, sizeof(server_recv), 0);
		if (ser_recv_size == filesize)
			goto exit;

		write_log(log_fd, "filename:%s,filemd5:%s,filesize:%ld\n", filename, filemd5, filesize);
		strcat(filesave_path, filename);

		if (access(filesave_path, R_OK) == -1)
			fp = fopen(filesave_path, "wb");
		else
			fp = fopen(filesave_path, "ab");

		if (fp == NULL)
			continue;

		//recv data from client
		bzero(buffer, MAXLENGTH);
		pos = 0, length = 0;
		recv_flag = false;

		while (1)
		{
			if (pos == filesize)
			{
				file_md5_value = MD5_file(filesave_path, MD5_MODE);
				int flag = strcmp(filemd5, file_md5_value);
				IFREE(file_md5_value);
				if (flag == 0)
				{
					send(connect_fd, "true", SOCKET_FLAG, 0);
					break;
				}
				else
				{
					send(connect_fd, "false", SOCKET_FLAG, 0);
					write_log(log_fd, "error:%s send failed, will be send again,in %d,at %s\n", filesave_path, __LINE__, __FUNCTION__);
					goto exit;
				}
			}
			else
			{
				if (recv_flag)//断点续传的点 当长度和flag都不符合，那么说明client断开了,或者就是已经传输完毕了
					break;
			}

			length = recv(connect_fd, buffer, MAXLENGTH, 0);
			
			if (length > 0)
			{
				pos += length;
				int write_length = fwrite(buffer, sizeof(char), length, fp);
				if (write_length < length)
				{
					write_log(log_fd, "error:%s fwrite failed,in %d,at %s\n", filename, __LINE__, __FUNCTION__);
					break;
				}
			}
			else
				recv_flag = true;

			fflush(fp);
			bzero(buffer, MAXLENGTH);
		}

		pthread_mutex_lock(&g_mutex_setflag);
		SetPrivateProfileInt(tmpserver, (char *)"default", filename, pos);
		pthread_mutex_unlock(&g_mutex_setflag);

		pthread_mutex_lock(&g_mutex_qwrite_hdfs);
		file_md5_value = MD5_file(filesave_path, MD5_MODE);
		if (!strcmp(filemd5, file_md5_value))
		{
			qwrite_hdfs.push(string(filesave_path));
			write_cmd("the file %s md5 was same\n", filename);
		}
		else
			write_log(log_fd, "smd5:%s,dmd5:%s,error:%s,transfer files md5 was diff,in %d,at %s\n", filemd5, file_md5_value, filename, __LINE__, __FUNCTION__);
		IFREE(file_md5_value);
		pthread_mutex_unlock(&g_mutex_qwrite_hdfs);

	exit:
		if (fp != NULL)
			fclose(fp);

		if (connect_fd != 0)
			close(connect_fd);
	}
}

void *write_files_thread(void *agrs)
{
	while (1)
	{
		pthread_mutex_lock(&g_mutex_qwrite_hdfs);
		queue <string> temp;
		if (!qwrite_hdfs.empty())
		{
			swap(temp, qwrite_hdfs);
			pthread_mutex_unlock(&g_mutex_qwrite_hdfs);

			while (!temp.empty())
			{
				write_HDFS(temp.front());
				temp.pop();
			}
		}
		else
			pthread_mutex_unlock(&g_mutex_qwrite_hdfs);

		sleep(10);
	}
}

int main(int argc, char** argv)
{
	int	socket_fd = 0, timeout = 1000, cpu_count = 0;
	struct sockaddr_in servaddr;
	string server_config = "";
	pthread_t queue_id;
	int reuse = 1;
	int local_port = 0;

	server_config = /*string(CONFIG_PATH) +*/ string(SERVER_CONF);
	cpu_count = GetPrivateProfileInt((char *)server_config.c_str(), (char *)"default", (char *)"cpu_count");
	save_path = GetPrivateProfileString((char *)server_config.c_str(), (char *)"default", (char *)"save_path");
	prefix = GetPrivateProfileString((char *)server_config.c_str(), (char *)"default", (char *)"prefix");
	local_port = GetPrivateProfileInt((char *)server_config.c_str(), (char *)"default", (char *)"local_port");
	log_path = GetPrivateProfileString((char *)server_config.c_str(), (char *)"log", (char *)"log_path");
	hdfs_path = GetPrivateProfileString((char *)server_config.c_str(), (char *)"hdfs", (char *)"hdfs_path");
	hdfs_addr = GetPrivateProfileString((char *)server_config.c_str(), (char *)"hdfs", (char *)"hdfs_addr");
	hdfs_port = GetPrivateProfileInt((char *)server_config.c_str(), (char *)"hdfs", (char *)"hdfs_port");
	
	if ((cpu_count < 0) || (log_path == NULL) || (hdfs_path == NULL)
		|| (hdfs_path == NULL) || (hdfs_port == 0) || (local_port == 0) || (save_path == NULL) || (prefix == NULL))
	{
		write_cmd("error:bad configure file,cpu_count:%d,save_path:%s,prefix:%s,log_path:%s,hdfs_path:%s,hdfs_addr:%s,hdfs_port:%d,local_port:%d,at %d,in %s\n", 
			cpu_count, save_path, prefix, log_path, hdfs_path, hdfs_addr, hdfs_port, local_port, __LINE__, __FUNCTION__);
		goto exit;
	}

	log_fd = log_file(log_path, (char *)"server", MINTUE_MODE);
	if (log_fd == NULL)
		goto exit;

	fs = hdfsConnect(hdfs_addr, hdfs_port);
	if (!fs)
	{
		write_log(log_fd, "error:failed to connect to hdfs,ip:%s,port:%d,at %d,in %s\n", 
			hdfs_addr, hdfs_port, __LINE__, __FUNCTION__);
		goto exit;
	}

	pthread_mutex_init(&g_mutex_setflag, 0);
	pthread_mutex_init(&g_mutex_qwrite_hdfs, 0);
	id = (pthread_t *)calloc(SERVER_COUNT, sizeof(pthread_t));
	if (id == NULL)
	{
		write_log(log_fd, "error:pthread_t id calloc failed,at %d,in:%s\n", __LINE__, __FUNCTION__);
		goto exit;
	}
	
	/* init Socket */
	if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		write_log(log_fd, "ERROR:create socket error:%s(errno:%d),at %d,in %s\n", 
			strerror(errno), errno, __LINE__, __FUNCTION__);
		goto exit;
	}
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse) );

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(local_port);

	//bind
	if(bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
	{
		write_log(log_fd, "error:bind socket error:%s(errno:%d),at %d,in %s\n", 
			strerror(errno), errno, __LINE__, __FUNCTION__);
		goto exit;
	}

	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
	
	//listen
	if(listen(socket_fd, 10) == -1)
	{
		write_log(log_fd, "error:listen socket error:%s(errno:%d),at %d,in %s\n", 
			strerror(errno), errno, __LINE__, __FUNCTION__);
		goto exit;
	}

	pthread_create(&queue_id, NULL, write_files_thread, NULL);

	for (int i = 0; i < cpu_count; ++i)
		pthread_create(&id[i], NULL, accept_files, (void *)&socket_fd);

	pthread_join(queue_id, NULL);
	for (int i = 0; i < cpu_count; ++i)
		pthread_join(id[i], NULL);

exit:
	
	if (socket_fd != 0)
		close(socket_fd);

	if (log_fd != NULL)
		close_file(log_fd);
	
	IFREE(hdfs_addr);
	IFREE(hdfs_path);
	IFREE(log_path);
	IFREE(save_path);
	IFREE(prefix);
	IFREE(id);
	if (fs != NULL)
		hdfsDisconnect(fs);

	pthread_mutex_destroy(&g_mutex_setflag);
	pthread_mutex_destroy(&g_mutex_qwrite_hdfs);

	return 0;
}
