#ifndef RDKAFKA_OPERATOR_H
#define RDKAFKA_OPERATOR_H

//#include <stdint.h>
#include <string>
/*
* Typically include path in a real application would be
* #include <librdkafka/rdkafkacpp.h>
*/
//#include "../../src-cpp/rdkafkacpp.h"
#include "librdkafka/rdkafkacpp.h"
#include "setlog.h"


static bool run = true;

class ExampleDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
	void dr_cb(RdKafka::Message &message) {
	}
};


class ExampleEventCb : public RdKafka::EventCb {
public:
	void event_cb(RdKafka::Event &event) {
		string errstr = "";
		switch (event.type())
		{
		case RdKafka::Event::EVENT_ERROR:
			if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
				run = false;
			errstr = "ERROR (" + RdKafka::err2str(event.err()) + "): " + event.str();
			break;

		case RdKafka::Event::EVENT_STATS:
			break;

		case RdKafka::Event::EVENT_LOG:
			break;

		default:
			break;
		}
	}
};

/* Use of this partitioner is pretty pointless since no key is provided
* in the produce() call. */
class MyHashPartitionerCb : public RdKafka::PartitionerCb {
public:
	int32_t partitioner_cb(const RdKafka::Topic *topic, const std::string *key,
		int32_t partition_cnt, void *msg_opaque) {
		return djb_hash(key->c_str(), key->size()) % partition_cnt;
	}
private:

	static inline unsigned int djb_hash(const char *str, size_t len) {
		unsigned int hash = 5381;
		for (size_t i = 0; i < len; i++)
			hash = ((hash << 5) + hash) + str[i];
		return hash;
	}
};

struct kafka_conrf_struct
{
	RdKafka::Topic *handle_topic;
	std::string broker_list;
	std::string topic;
};
typedef struct kafka_conrf_struct KAFKA_CONF;


class KafkaServer {
public:
	static KafkaServer & getInstance() {
		static KafkaServer instance; // 局部静态变量
		return instance;
	}
public:
	int connectServer(KAFKA_CONF* kafka_conf);

	int sendMessage(KAFKA_CONF* kafka_conf, std::string& line);

	void disconnectServer(KAFKA_CONF* kafka_conf);

	~KafkaServer();

private:
	KafkaServer();
	KafkaServer(const KafkaServer &);
	KafkaServer & operator = (const KafkaServer &);

	RdKafka::Conf *m_conf;
	RdKafka::Conf *m_tconf;
	std::string err_str;
	ExampleDeliveryReportCb ex_dr_cb;
	ExampleEventCb ex_event_cb;
	bool flag;
	RdKafka::Producer *m_producer;
};

void init_topic_struct(KAFKA_CONF* kafkaconf);
static void sigterm(int sig);

#endif
