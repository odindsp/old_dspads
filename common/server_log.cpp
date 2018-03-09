#include <pthread.h>
#include <stdlib.h>
#include "errorcode.h"
#include "confoperation.h"
#include "setlog.h"
#include "rdkafka_operator.h"
#include "server_log.h"


SERVER_LOG g_ser_log;

SERVER_LOG::SERVER_LOG()
{
	log_bid_ = NULL;
	log_detail_ = NULL;
	using_bid_kafka_ = using_detail_kafka_ = true;
}

SERVER_LOG::~SERVER_LOG()
{
	if (log_bid_)
	{
		delete[] log_bid_;
		log_bid_ = NULL;
	}

	if (log_detail_)
	{
		delete[] log_detail_;
		log_detail_ = NULL;
	}
}

int SERVER_LOG::init(int count_cpu, int adx, uint64_t logid, const char* conf_file, bool *run_flag)
{
	if (count_cpu <= 0 || adx == 0 || logid == 0 || conf_file == NULL || run_flag == NULL){
		return E_UNKNOWN;
	}
	adx_ = adx;

	// 日志处理方式
	using_hdfs_ = GetPrivateProfileInt(conf_file, "default", "hdfslog");
	using_bid_kafka_ = using_kafka(conf_file, "bid_server");
	using_detail_kafka_ = using_kafka(conf_file, "detail_server");

	log_bid_ = new DATATRANSFER[count_cpu];
	if (log_bid_ == NULL){
		return E_MALLOC;
	}

#ifdef DETAILLOG
	log_detail_ = new DATATRANSFER[count_cpu];
	if (log_detail_ == NULL){
		return E_MALLOC;
	}
#endif

	// 1 获取IP及端口号
	string bid_ser_ip, detail_ser_ip;
	KAFKA_CONF kafka_conf_bid, kafka_conf_detail;
	uint32_t bid_ser_port, detail_ser_port;
	char *val = NULL;
	init_topic_struct(&kafka_conf_bid);
	init_topic_struct(&kafka_conf_detail);

	if (!using_bid_kafka_)
	{
		val = GetPrivateProfileString(conf_file, "logserver", "bid_ip");
		if (val == NULL){
			return E_INVALID_PROFILE_BIDIP;
		}
		bid_ser_ip = val;
		free(val);

		bid_ser_port = GetPrivateProfileInt(conf_file, "logserver", "bid_port");
		if (bid_ser_port == 0){
			return E_INVALID_PROFILE_BIDPORT;
		}
	}
	else
	{
		val = GetPrivateProfileString(conf_file, "logserver", "bid_broker_list");
		if (val == NULL){
			return E_INVALID_PROFILE_BIDBROKER;
		}
		kafka_conf_bid.broker_list = val;
		free(val);

		val = GetPrivateProfileString(conf_file, "logserver", "bid_topic");
		if (val == NULL){
			return E_INVALID_PROFILE_BIDTOPIC;
		}
		kafka_conf_bid.topic = val;
		free(val);
	}
	
#ifdef DETAILLOG
	if (!using_detail_kafka_)
	{
		val = GetPrivateProfileString(conf_file, "logserver", "detail_ip");
		if (val == NULL)
		{
			return E_INVALID_PROFILE_DETAILIP;
		}
		detail_ser_ip = val;
		free(val);

		detail_ser_port = GetPrivateProfileInt(conf_file, "logserver", "detail_port");
		if (detail_ser_port == 0)
		{
			return E_INVALID_PROFILE_DETAILPORT;
		}
	}
	else
	{
		val = GetPrivateProfileString(conf_file, "logserver", "detail_broker_list");
		if (val == NULL){
			return E_INVALID_PROFILE_DETAILBROKER;
		}
		kafka_conf_detail.broker_list = val;
		free(val);
		val = GetPrivateProfileString(conf_file, "logserver", "detail_topic");
		if (val == NULL){
			return E_INVALID_PROFILE_DETAILTOPIC;
		}
		kafka_conf_detail.topic = val;
		free(val);
	}
#endif

	// 2 连接服务器
	for (uint8_t i = 0; i < count_cpu; ++i)
	{
		log_bid_[i].write = using_hdfs_;
		log_bid_[i].kafka = using_bid_kafka_;
		if (log_bid_[i].write)
		{
			vector<pthread_t> thread_id;
			log_bid_[i].fd = openLog(conf_file, "loghdfs", true, run_flag, thread_id, true);
			if (log_bid_[i].fd == 0)
			{
				return E_LOG_SERVER_HDFS;
			}
			log_bid_[i].logid = logid;
			log_bid_[i].threadlog = thread_id.size()>0 ? thread_id[0] : 0;
		}
		else if (using_bid_kafka_)
		{
			if (!connect_log_server(logid, &kafka_conf_bid, run_flag, &log_bid_[i]))
			{
				if (log_bid_[i].threadlog != -1){
					pthread_join(log_bid_[i].threadlog, NULL);
				}
				disconnect_log_server(&log_bid_[i],true);
				return E_LOG_SERVER_BIDKAFKA;
			}
		}
		else
		{
			if (!connect_log_server(logid, bid_ser_ip.c_str(), bid_ser_port, run_flag, &log_bid_[i]))
			{
				if (log_bid_[i].threadlog != -1)
				{
					pthread_join(log_bid_[i].threadlog, NULL);
				}
				disconnect_log_server(&log_bid_[i]);
				return E_LOG_SERVER;
			}
		}

		thread_ids_.push_back(log_bid_[i].threadlog);
		usleep(2000);

#ifdef DETAILLOG
		log_detail_[i].kafka = using_detail_kafka_;
		if (using_detail_kafka_)
		{
			if (!connect_log_server(logid, &kafka_conf_detail, run_flag, &log_detail_[i]))
			{
				if (log_detail_[i].threadlog != -1){
					pthread_join(log_detail_[i].threadlog, NULL);
				}
				disconnect_log_server(&log_detail_[i], true);
				return E_LOG_SERVER_DETAILKAFKA;
			}
		}
		else
		{
			if (!connect_log_server(logid, detail_ser_ip.c_str(), detail_ser_port, run_flag, &log_detail_[i]))
			{
				if (log_detail_[i].threadlog != -1)
				{
					pthread_join(log_detail_[i].threadlog, NULL);
				}
				disconnect_log_server(&log_detail_[i]);
				return E_LOG_SERVER_DETAIL;
			}
		}

		thread_ids_.push_back(log_detail_[i].threadlog);
		usleep(2000);
#endif
	}

	return E_SUCCESS;
}

void SERVER_LOG::uninit()
{
	for (uint8_t i = 0; i < count_; ++i)
	{
		if (log_bid_ != NULL)
		{
			if (log_bid_[i].write && log_bid_[i].fd)
			{
				closeLog(log_bid_[i].fd);
			}
			else
			{
				disconnect_log_server(&log_bid_[i], using_bid_kafka_);
			}
		}

#ifdef DETAILLOG
		if (log_detail_ != NULL)
		{
			disconnect_log_server(&log_detail_[i], using_detail_kafka_);
		}
#endif
	}

	for (uint8_t i = 0; i < thread_ids_.size(); ++i)
	{
		pthread_join(thread_ids_[i], NULL);
	}
}

void SERVER_LOG::send_detail_log(int index, const string& detail_flow)
{
#ifdef DETAILLOG
	if (using_detail_kafka_){
		sendLog(&log_detail_[index], string("|") + detail_flow);
	}
	else{
		sendLog(&log_detail_[index], detail_flow);
	}
#endif
}

void SERVER_LOG::send_bid_log(int index, const COM_REQUEST &commrequest, const COM_RESPONSE &cresponse, int err)
{
	string data = BidInfo2String(commrequest, cresponse, adx_, err);

	DATATRANSFER *datatransfer = &log_bid_[index];
	if (log_bid_[index].write){
		writeLog(datatransfer->fd, data);
	}
	else
	{
		if (datatransfer->kafka){
			data = string("\t") + data;
		}
		sendLog(datatransfer, data);
	}
}

bool SERVER_LOG::using_kafka(const char* conf_file, const char *log_type)
{
	string tmp;
	char *val = GetPrivateProfileString(conf_file, "logserver", log_type);
	if (val != NULL)
	{
		tmp = val;
		free(val);
	}

	if (tmp.empty()){
		return true;
	}

	trimstring(tmp);
	return strcasecmp(tmp.c_str(), "kafka") == 0;
}
