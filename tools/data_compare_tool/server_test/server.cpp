/*
 * server.cpp
 *
 *  Created on: 2016Äê10ÔÂ10ÈÕ
 *      Author: zhangzheng
 */
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include "common/common.h"
#include "server.h"

using namespace std;

#define 	CONFIG_PATH			"/etc/dspads_odin/"
#define		SERVER_CONF			"server.conf"


#define SERVER_PORT    4000
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024

FILE *log_fd = NULL;
char *log_path = NULL;
char *data_path = NULL;
char *outdata_path = NULL;

char* trim(char* s)
{
    char* z = 0;
    char* e = 0;
//    while(*s != 0 && isspace(*s))
//        ++s;
    z = s;
        e = z;
    while(*z != 0)
    {
        if(!isspace(*z))
            e = ++z;
        else
            ++z;
    }
    *e = 0;
    return s;
}

bool mySystem(const char *command)
{
    int status;
    status = system(command);

    if (-1 == status)
    {
    	write_log(log_fd,"mySystem: system error!");
        return false;
    }
    else
    {
        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                return true;
            }
            write_log(log_fd,"mySystem: run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
            return false;
        }
        write_log(log_fd,"mySystem: exit status = [%d]\n", WEXITSTATUS(status));
        return false;
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    int server_socket;
    int server_port;
    int opt = 1;
    int timeout = 3000;
    string server_config = "";

    int keepAlive = 1;
    int keepIdle = 60;
    int keepInterval = 5;
    int keepCount = 3;
    vector<string> v_email;
	char buf_path[MIDLENGTH] = "";



    server_config = string(CONFIG_PATH) + string(SERVER_CONF);

	log_path = GetPrivateProfileString((char *)server_config.c_str(), (char *)"log", (char *)"log_path");
	server_port = GetPrivateProfileInt((char *)server_config.c_str(), (char *)"default", (char *)"server_port");
	data_path = GetPrivateProfileString((char *)server_config.c_str(), (char *)"data", (char *)"data_path");
	outdata_path = GetPrivateProfileString((char *)server_config.c_str(), (char *)"data", (char *)"outdata_path");

	if(log_path == NULL)
	{
		goto exit;
	}

	log_fd = log_file(log_path, (char *)"server", MINTUE_MODE);
	if (log_fd == NULL)
		goto exit;

	sprintf(buf_path, "mkdir -p %s", data_path);
	cout << "data_path:" << buf_path << endl;
	system(buf_path);

	memset(buf_path,0,MIDLENGTH);
	sprintf(buf_path, "mkdir -p %s", outdata_path);
	cout << "outdata_path:" << buf_path << endl;
	system(buf_path);

    bzero(&server_addr,sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    // create a socket
    server_socket = socket(PF_INET,SOCK_STREAM,0);
    if( server_socket < 0)
    {
		write_log(log_fd, "Create Socket Failed ,at %d,in:%s\n", __LINE__, __FUNCTION__);
        exit(1);
    }

    // bind socket to a specified address
    setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(server_socket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(server_socket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(server_socket, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
		write_log(log_fd, "Server Bind Port %d Failed!\n",server_port);
		printf("Server Bind Port %d Failed!\n");
        exit(1);
    }

    // listen a socket
    if(listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
		write_log(log_fd, "Server Listen Failed!,at %d,in:%s\n", __LINE__, __FUNCTION__);
        exit(1);
    }

    // run server
    while (1)
    {
        struct sockaddr_in client_addr;
        int client_socket;
        socklen_t length;
        char buffer[BUFFER_SIZE] = "";
        char buf_param[MIDLENGTH] = "";
        char buf_param1[MIDLENGTH] = "";
        char buf_file[MIDLENGTH] = "";
        char buf_change_file[MAXLENGTH];
        char sendmail[MAXLENGTH] = "";

        char dir_buf[MIDLENGTH] = "";
        char dir_buf1[MIDLENGTH] = "";
		char creat_file[MIDLENGTH] = "";

        string date = "",type = "",groupid = "",adid = "",filename = "",target = "";

        // accept socket from client
        length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if( client_socket < 0)
        {
    		write_log(log_fd, "Server Accept Failed!,at %d,in:%s\n", __LINE__, __FUNCTION__);
            break;
        }

        // receive data from client
        while(1)
        {
            bzero(buffer, BUFFER_SIZE);
            length = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (length < 0)
            {
        		write_log(log_fd, "Server Recieve Data Failed!\n");
                break;
            }
            else if (length == 0)
            {
        		write_log(log_fd, "Client  colsed  connection!\n");
                break;
            }

            if('q' == buffer[0])
            {
                printf("Quit from client!\n");
                break;
            }
            write_log(log_fd, "*****************************************************data coming\n");
            write_log(log_fd, "Receive Date  size=%d string = %s\n",length,buffer);


    		string str = trim(buffer);
    		int pos = str.rfind(" ");
    		string str_email = str.substr(pos+1);

            int pos_m = str_email.find(",");
            while (pos_m != string::npos)
            {
            	string sub_email = str_email.substr(0,pos_m);
            	v_email.push_back(sub_email);
            	str_email = str_email.substr(pos_m+1);
            	pos_m = str_email.find(",");
            }
        	v_email.push_back(str_email);

            write_log(log_fd, "Receive email people size=%d send to %s \n",v_email.size(),str_email.c_str());

            string use_data = str.substr(0,pos);
            write_log(log_fd, "Receive use data string = %s\n",use_data.c_str());

            string str_param = use_data;
            int p_pos;
            for(int i = 0; i < 7; ++i)
            {
            	p_pos = str_param.find(" ");
            	if(p_pos != string::npos)
            	{
            		if(i == 0)
            		{
            	        date = str_param.substr(0,p_pos);
            		}
            		else if(i == 1)
            		{
            			type = str_param.substr(0,p_pos);

            		}
            		else if(i == 2)
            		{
            			groupid = str_param.substr(0,p_pos);
            		}
            		else if(i == 4)
            		{
            			adid = str_param.substr(0,p_pos);
            		}
            		else if(i == 5)
            		{
            			filename = str_param.substr(0,p_pos);
            		}
            		str_param = str_param.substr(p_pos+1);
            	}

            }
            target = str_param;
            sprintf(buf_param,"%s_%s_%s_%s_%s_adx",date.c_str(),type.c_str(),groupid.c_str(),adid.c_str(),target.c_str());
            sprintf(buf_param1,"%s.txt",buf_param);
            write_log(log_fd, "Receive  file %s ,mail send file is %s,\n",filename.c_str(),buf_param1);

            bool bRet = false;
            sleep(5);
            sprintf(buf_file,"./ftp.sh %s %s",filename.c_str(), data_path);
            write_log(log_fd, "ftp get file cmd : %s\n",buf_file);
            //system(buf_file);
            bRet = mySystem(buf_file);

            while(bRet != true)
            {
            	write_log(log_fd, "ftp shell %s get file faild! \n",buf_file);
            	bRet = mySystem(buf_file);
            }

            sleep(5);
            sprintf(buf_change_file,"iconv -f gb2312 -t utf-8 %s%s > %stmp.csv",
            		data_path,filename.c_str(),data_path);
        	write_log(log_fd, "exchange unicode cmd is: %s \n",buf_change_file);

            bRet = mySystem(buf_change_file);
            int n = 0;
            bool flag = true;
            while(bRet != true)
            {
            	if(n > 10)
            	{
                	flag = false;
                	break;
            	}
            	write_log(log_fd, "cmd : %s exec faild \n",buf_change_file);
            	mySystem(buf_file);
            	sleep(2);
            	write_log(log_fd, "ftp shell %s get file again! \n",buf_file);
                bRet = mySystem(buf_change_file);
                ++n;
            }

            if(flag != true)
            {
            	write_log(log_fd, "please check network and try agine \n");
            	break;
            }
            memset(buf_change_file,0,MAXLENGTH);
            sprintf(buf_change_file,"sed -i 's/\r$//' %stmp.csv;mv %stmp.csv %s%s",
            		data_path,data_path,data_path,filename.c_str());
        	write_log(log_fd, "cmd is: %s \n",buf_change_file);
        	if(!mySystem(buf_change_file))
        	{
            	write_log(log_fd, "cmd : %s exec faild \n",buf_change_file);
        	}

            string cc_mails = "";
            for(int i = 1; i < v_email.size(); ++i)
            {
            	//cc_mails = cc_mails + " -c " + v_email[i];
            	cc_mails = cc_mails + " " + v_email[i];
            }
            string s_mail = v_email[0];

            sprintf(dir_buf,"%s%s",outdata_path,buf_param);
            sprintf(dir_buf1,"%s%s",outdata_path,buf_param1);

            string file = "test.txt";

			sprintf(creat_file,"touch %s",dir_buf);
			write_log(log_fd, "cmd is: %s \n",creat_file);
			//system(creat_file);
			bRet = mySystem(creat_file);
			if(bRet != true)
			{
				write_log(log_fd, "cmd :%s is exec failed\n",creat_file);
			}

			memset(sendmail,0,MAXLENGTH);
			sprintf(sendmail,"cp %s %s;mail -a %s -s 'Data compare result file' %s %s < %s",dir_buf,dir_buf1,dir_buf1,s_mail.c_str(),cc_mails.c_str(),file.c_str());
			write_log(log_fd, "cmd is: %s \n",sendmail);
			//system(sendmail);
			bRet = mySystem(sendmail);
			if(bRet != true)
			{
				write_log(log_fd, "cmd :%s is exec failed\n",sendmail);
			}

			memset(sendmail,0,MAXLENGTH);
			sleep(2);
			sprintf(sendmail,"rm %s",dir_buf1);
			write_log(log_fd, "cmd is: %s \n",sendmail);
			system(sendmail);

            if(s_mail != "")
            {
            	s_mail = "";
            }
            if(cc_mails != "")
            {
            	cc_mails = "";
            }

            v_email.clear();
//            int nRet;
//            nRet = send(client_socket, buffer, sizeof(buffer), 0);
//            write_log(log_fd, "server send  Date : %s\n",buffer);
        }

        close(client_socket);
    }

    close(server_socket);

exit:
	if (log_fd != NULL)
		close_file(log_fd);

	if (log_path != NULL)
		log_path = NULL;

	if(server_socket != 0)
	{
		close(server_socket);
	}

    return 0;
}



