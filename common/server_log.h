#ifndef _SERVER_LOG_H_
#define _SERVER_LOG_H_
#include <vector>
#include "util.h"
#include "bid_filter_type.h"
using namespace std;


// 0 发送两种日志：流量日志、竞价详细日志（描述了不出价原因）。
// 1 流量日志可能直接写到本地文件，然后打包发送到hdfs，如谷歌这么处理。
// 2 竞价详情日志的数据较长，会导致服务器处理不过来，
//   运维可能根据实际情况，决定有些竞价引擎不开启该功能（编译时去掉Makefile中的DETAILLOG宏定义）。
// 3 日志可以选择发送到flume还是kafka
class SERVER_LOG
{
public:
	SERVER_LOG();
	~SERVER_LOG();

	int init(int count_cpu, int adx, uint64_t logid, const char* conf_file, bool *run_flag);

	void send_detail_log(int index, const string& detail_flow);
	void send_bid_log(int index, const COM_REQUEST &commrequest, const COM_RESPONSE &cresponse, int err);

	void uninit();

protected:
	bool using_kafka(const char* conf_file, const char *log_type);

private:
	int count_;
	int adx_;
	bool using_hdfs_;
	bool using_bid_kafka_;
	bool using_detail_kafka_;
	vector<pthread_t> thread_ids_;
	
	DATATRANSFER *log_bid_;
	DATATRANSFER *log_detail_;
};

extern SERVER_LOG g_ser_log;

#endif
