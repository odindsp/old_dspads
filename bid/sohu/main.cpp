#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <syslog.h>
#include <errno.h>
#include <fstream>
#include "sohu_response.h"
using namespace std;


#define SOHU_CONF		"dsp_sohu.conf"
#define APPCATTABLE		"appcat_sohu.txt"
#define TEMPLATETABLE   "template_sohu.txt"
#define ADVCATTABLE     "advcat_sohu.txt"


uint64_t g_logid_response, g_logid_local, g_logid;
uint64_t cpu_count = 0;
pthread_t *id = NULL;
uint64_t g_ctx = 0;
uint64_t geodb = 0;

map<int64, vector<uint32_t> > sohu_app_cat_table;
map<string, vector<pair<string, COM_NATIVE_REQUEST_OBJECT> > > sohu_template_table;
map<string, vector<uint32_t> > adv_cat_table_adx2com;

bool is_print_time = false;
bool fullreqrecord = false;

bool run_flag = true;
int *numcount = NULL;


void sigroutine(int dunno)
{
	switch (dunno) {
	case SIGINT://SIGINT
	case SIGTERM://SIGTERM
		syslog(LOG_INFO, "Get a signal -- %d", dunno);
		run_flag = false;
		break;
	default:
		break;
	}
}

void splitString(string &text, vector<string> &parts, const string &delimiter = " ")
{
	parts.clear();
	size_t delimiterPos = text.find(delimiter);
	size_t lastPos = 0;
	if (delimiterPos == string::npos)
	{
		parts.push_back(text);
		return;
	}

	while (delimiterPos != string::npos)
	{
		parts.push_back(text.substr(lastPos, delimiterPos - lastPos));
		lastPos = delimiterPos + delimiter.size();
		delimiterPos = text.find(delimiter, lastPos);
	}
	parts.push_back(text.substr(lastPos));
}

bool init_template_table(IN const string file_path)
{
	bool parsesuccess = false;
	ifstream file_in;
	if (file_path.length() == 0)
	{
		va_cout("file path is empty");
		goto exit;
	}

	file_in.open(file_path.c_str(), ios_base::in);
	if (file_in.is_open())
	{
		string str = "";
		while (getline(file_in, str))
		{
			string val, key;

			trim_left_right(str);

			if (str[0] != '#')
			{
				int pos_equal = str.find('=');
				if (pos_equal == string::npos)
					continue;

				key = str.substr(0, pos_equal);
				val = str.substr(pos_equal + 1);

				trim_left_right(val);
				trim_left_right(key);

				if (val.length() == 0)
					continue;
				if (key.length() == 0)
					continue;
				vector<string> parts;
				splitString(val, parts, ",");
				if (parts.size() != 4)
				{
					cout << "wrong vector size" << endl;
					continue;
				}
				cout << parts[0] << "," << parts[1] << "," << parts[2] << "," << parts[3] << endl;
				COM_NATIVE_REQUEST_OBJECT native;
				init_com_native_request_object(native);
				native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
				native.plcmtcnt = 1;
				// 0 imgsize, 1 titlelen, 2 desclen, 3 iconsize
				int w = 0, h = 0;
				sscanf(parts[0].c_str(), "%dx%d", &w, &h);
				if (w != 0 && h != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset_img;
					init_com_asset_request_object(asset_img);
					asset_img.id = 3;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset_img.required = 1;   // must have
					asset_img.type = NATIVE_ASSETTYPE_IMAGE;
					asset_img.img.type = ASSET_IMAGETYPE_MAIN;
					asset_img.img.w = w;
					asset_img.img.h = h;
					native.assets.push_back(asset_img);
				}

				sscanf(parts[3].c_str(), "%dx%d", &w, &h);
				if (w != 0 && h != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset_img;
					init_com_asset_request_object(asset_img);
					asset_img.id = 2;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset_img.required = 1;   // must have
					asset_img.type = NATIVE_ASSETTYPE_IMAGE;
					asset_img.img.type = ASSET_IMAGETYPE_ICON;
					asset_img.img.w = w;
					asset_img.img.h = h;
					native.assets.push_back(asset_img);
				}

				int titlelen = atoi(parts[1].c_str());
				if (titlelen != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset;
					init_com_asset_request_object(asset);
					asset.id = 1;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset.required = 1;   // must have
					asset.type = NATIVE_ASSETTYPE_TITLE;
					asset.title.len = titlelen * 3;
					native.assets.push_back(asset);
				}

				int desclen = atoi(parts[2].c_str());
				if (desclen != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset;
					init_com_asset_request_object(asset);
					asset.id = 4;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset.required = 1;   // must have
					asset.type = NATIVE_ASSETTYPE_DATA;
					asset.data.type = ASSET_DATATYPE_DESC;
					asset.data.len = desclen * 3;
					native.assets.push_back(asset);
				}
				sohu_template_table[key].push_back(make_pair(parts[0], native));
			}
		}

		parsesuccess = true;

		file_in.close();
	}
	else
	{
		va_cout("open config file error: %s", strerror(errno));
	}

exit:

	return parsesuccess;
}

static void *doit(void *arg)
{
	int errcode = E_SUCCESS;
	uint8_t index = (uint64_t)arg;
	FCGX_Request request;
	uint32_t contentlength = 0;
	int rc;
	FLOW_BID_INFO flow_bid(ADX_SOHU);

	FCGX_InitRequest(&request, 0, 0);

	while (run_flag)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
		static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;
		RECVDATA *recvdata = NULL;
		string senddata = "";

		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		flow_bid.reset();

		if (rc < 0)
			goto fcgi_finish;

		if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", request.envp)) == 0)
		{
			if (strcmp("application/x-protobuf", FCGX_GetParam("CONTENT_TYPE", request.envp)) == 0)
			{
				contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
				recvdata = (RECVDATA *)malloc(sizeof(RECVDATA));
				if (recvdata == NULL)
				{
					writeLog(g_logid_local, LOGERROR, "recvdata:alloc memory failed,at:%s,in:%d", __FUNCTION__, __LINE__);
					flow_bid.set_err(E_MALLOC);
					goto fcgi_finish;
				}

				recvdata->length = contentlength;
				recvdata->data = (char *)calloc(1, contentlength + 1);
				if (recvdata->data == NULL)
				{
					writeLog(g_logid_local, LOGERROR, "recvdata->data:alloc memory failed,at:%s,in:%d", __FUNCTION__, __LINE__);
					flow_bid.set_err(E_MALLOC);
					goto fcgi_finish;
				}

				if (FCGX_GetStr(recvdata->data, contentlength, request.in) == 0)
				{
					free(recvdata->data);
					free(recvdata);
					recvdata->data = NULL;
					recvdata = NULL;
					writeLog(g_logid_local, LOGERROR, "FCGX_GetStr failed,at:%s,in:%d", __FUNCTION__, __LINE__);
					flow_bid.set_err(E_REQUEST_HTTP_BODY);
					goto fcgi_finish;
				}
				if (fullreqrecord)
					writeLog(g_logid_local, LOGDEBUG, "recvdata= " + string(recvdata->data, recvdata->length));

				errcode = sohuResponse(index, g_ctx, recvdata, senddata, flow_bid);
				if (senddata != "")
				{
					pthread_mutex_lock(&context_mutex);
					FCGX_PutStr(senddata.data(), senddata.size(), request.out);
					pthread_mutex_unlock(&context_mutex);
				}
			}
			else
			{
				writeLog(g_logid_local, LOGERROR, "Not application/x-protobuf");
				flow_bid.set_err(E_REQUEST_HTTP_CONTENT_TYPE);
			}

		}
		else
		{
			char *requestparam = FCGX_GetParam("QUERY_STRING", request.envp);
			if (requestparam != NULL && strstr(requestparam, "pxene_debug=1") != NULL)
			{
				string data;
				get_server_content(g_ctx, index, data);
				pthread_mutex_lock(&context_mutex);
				FCGX_PutStr(data.data(), data.length(), request.out);
				pthread_mutex_unlock(&context_mutex);
			}

			writeLog(g_logid_local, LOGERROR, "Not POST.");
			flow_bid.set_err(E_REQUEST_HTTP_METHOD);
		}

	fcgi_finish:
		g_ser_log.send_detail_log(index, flow_bid.to_string());
		FCGX_Finish_r(&request);
	}

exit:
	cflog(g_logid_local, LOGINFO, "leave fn:%s, ln:%d", __FUNCTION__, __LINE__);
	cout << "doit " << (int)index << " exit" << endl;
	return NULL;
}

int main(int argc, char *argv[])
{
	int errcode = E_SUCCESS;
	bool is_textdata = false, is_textdata_log = false, is_textdata_response = false;
	map<int64, vector<uint32_t> >::iterator it;
	string sohu_appcat = "", sohu_private_conf = "", sohu_global_conf = "", sohu_template;
	char *private_conf = NULL, *global_conf = NULL;
	char *ipbpath = NULL;
	vector<pthread_t> thread_id;
	map<string, vector<uint32_t> >::iterator it_advcat;

	FCGX_Init();

	sohu_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
	global_conf = (char *)sohu_global_conf.c_str();

	cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
	if (cpu_count <= 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_CPU_COUNT;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	ipbpath = GetPrivateProfileString(global_conf, (char *)"default", (char *)"ipb");
	if (ipbpath == NULL)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PROFILE_IPB;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	geodb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
	if (geodb == 0)
	{
		FAIL_SHOW;
		errcode = E_FILE_IPB;
		run_flag = false;
		goto exit;
	}

	id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));
	if (id == NULL)
	{
		FAIL_SHOW;
		errcode = E_MALLOC;
		run_flag = false;
		goto exit;
	}

	numcount = (int *)calloc(cpu_count, sizeof(int));
	if (numcount == NULL)
	{
		FAIL_SHOW;
		errcode = E_MALLOC;
		run_flag = false;
		goto exit;
	}
	sohu_private_conf = string(GLOBAL_PATH) + string(SOHU_CONF);

	private_conf = (char *)sohu_private_conf.c_str();

	is_textdata = GetPrivateProfileInt(private_conf, "locallog", "is_textdata");
	fullreqrecord = GetPrivateProfileInt(private_conf, "locallog", "fullreqrecord");
	is_textdata_log = GetPrivateProfileInt(private_conf, "log", "is_textdata");
	is_textdata_response = GetPrivateProfileInt(private_conf, "logresponse", "is_textdata");

	is_print_time = GetPrivateProfileInt(private_conf, "locallog", "is_print_time");
	va_cout("locallog is_print_time: %d", is_print_time);

	g_logid_response = openLog(private_conf, "logresponse", is_textdata_response, &run_flag, thread_id, is_print_time);
	g_logid_local = openLog(private_conf, "locallog", is_textdata, &run_flag, thread_id, is_print_time);
	g_logid = openLog(private_conf, "log", is_textdata_log, &run_flag, thread_id, is_print_time);
	if (g_logid == 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PARAM_LOGID;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}
	if (g_logid_local == 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PARAM_LOGID_LOC;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}
	if (g_logid_response == 0)
	{
		FAIL_SHOW;
		errcode = E_INVALID_PARAM_LOGID_RESP;
		va_cout("read profile file failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	signal(SIGINT, sigroutine);
	signal(SIGTERM, sigroutine);

	sohu_appcat = string(GLOBAL_PATH) + string(APPCATTABLE);

	if (!init_category_table_t(sohu_appcat, callback_insert_pair_string, (void *)&sohu_app_cat_table, false))
	{
		FAIL_SHOW;
		errcode = E_PROFILE_INIT_APPCAT;
		writeLog(g_logid_local, LOGERROR, "init_category_table_t failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	for (it = sohu_app_cat_table.begin(); it != sohu_app_cat_table.end(); ++it)
	{
		string val;
		for (int i = 0; i < it->second.size(); i++)
		{
			val += " " + hexToString(it->second[i]);
		}

		cout << "dump app: " << it->first << " -> " << val << endl;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(), 
		callback_insert_pair_string_hex, (void *)&adv_cat_table_adx2com, false))
	{
		FAIL_SHOW;
		cflog(g_logid_local, LOGERROR, "init adv_cat_table_adx2com table failed!");
		errcode = E_PROFILE_INIT_ADVCAT;
		run_flag = false;
		goto exit;
	}

	for (it_advcat = adv_cat_table_adx2com.begin(); it_advcat != adv_cat_table_adx2com.end(); ++it_advcat)
	{
		string val;
		for (int i = 0; i < it_advcat->second.size(); i++)
		{
			val += " " + hexToString(it_advcat->second[i]);
		}

		va_cout("dump adv in: %s -> %s", it_advcat->first.c_str(), val.c_str());
	}

	sohu_template = string(GLOBAL_PATH) + string(TEMPLATETABLE);
	if (!init_template_table(sohu_template))
	{
		FAIL_SHOW;
		errcode = E_PROFILE_INIT_TEMPLATE;
		writeLog(g_logid_local, LOGERROR, "init_template_table failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	errcode = bid_filter_initialize(g_logid_local, ADX_SOHU, cpu_count, &run_flag, private_conf, thread_id, &g_ctx);
	if (errcode != E_SUCCESS)
	{
		FAIL_SHOW;
		writeLog(g_logid_local, LOGERROR, "bid_filter_initialize failed,err:0x%x,at:%s,in:%d", errcode, __FUNCTION__, __LINE__);
		run_flag = false;
		goto exit;
	}

	errcode = g_ser_log.init(cpu_count, ADX_SOHU, g_logid_local, private_conf, &run_flag);
	if (errcode != E_SUCCESS)
	{
		FAIL_SHOW;
		run_flag = false;
		goto exit;
	}

	for (uint8_t i = 0; i < cpu_count; ++i)
	{
		pthread_create(&id[i], NULL, doit, (void *)i);
	}

	for (uint8_t i = 0; i < thread_id.size(); ++i)
	{
		pthread_join(thread_id[i], NULL);
	}

exit:
	g_ser_log.uninit();

	if (g_logid_local != 0)
		closeLog(g_logid_local);

	if (g_logid != 0)
		closeLog(g_logid);

	if (g_logid_response != 0)
		closeLog(g_logid_response);

	if (geodb != 0)
	{
		closeIPB(geodb);
		geodb = 0;
	}
	if (numcount)
	{
		free(numcount);
		numcount = NULL;
	}

	if (g_ctx != 0)
		bid_filter_uninitialize(g_ctx);

	if (errcode == E_SUCCESS)
	{
		for (uint8_t i = 0; i < cpu_count; ++i)
		{
			pthread_kill(id[i], SIGKILL);
		}
	}

	if (id != NULL)
		free(id);

	cout << "main exit, errcode:0x" << hex << errcode << endl;

	return errcode;
}
