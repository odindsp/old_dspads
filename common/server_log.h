#ifndef _SERVER_LOG_H_
#define _SERVER_LOG_H_
#include <vector>
#include "util.h"
#include "bid_filter_type.h"
using namespace std;


// 0 ����������־��������־��������ϸ��־�������˲�����ԭ�򣩡�
// 1 ������־����ֱ��д�������ļ���Ȼ�������͵�hdfs����ȸ���ô����
// 2 ����������־�����ݽϳ����ᵼ�·���������������
//   ��ά���ܸ���ʵ�������������Щ�������治�����ù��ܣ�����ʱȥ��Makefile�е�DETAILLOG�궨�壩��
// 3 ��־����ѡ���͵�flume����kafka
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
