/*
 * librdkafka - Apache Kafka C library
 *
 * Copyright (c) 2014, Magnus Edenhill
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Apache Kafka consumer & producer example programs
 * using the Kafka driver from librdkafka
 * (https://github.com/edenhill/librdkafka)
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#include <getopt.h>

#include "rdkafka_operator.h"

static void sigterm (int sig) {
  run = false;
}

KafkaServer::KafkaServer() {
	m_conf = NULL;
	m_tconf = NULL;
	m_producer = NULL;
	err_str = "";
	flag = false;
	/*
	* Create configuration objects
	*/
	m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
	m_tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

	m_conf->set("event_cb", &ex_event_cb, err_str);

	/*signal(SIGINT, sigterm);
	signal(SIGTERM, sigterm);*/

	/* Set delivery report callback */
	//conf->set("dr_cb", &ex_dr_cb, err_str);
}

int KafkaServer::connectServer(KAFKA_CONF* kafka_conf) {
	/*
	* Set configuration properties,make sure m_producer is unique in the same broker_list
	*/
	if (!flag) {
		m_conf->set("metadata.broker.list", kafka_conf->broker_list, err_str);
		m_producer = RdKafka::Producer::create(m_conf, err_str);
		if (!m_producer) {
			std::cerr << "Failed to create producer: " << err_str << std::endl;
			//exit(1);
			return -1;
		}
		//std::cout << "% Created producer " << kafka_conf->producer->name() << std::endl;
		flag = true;
	}
	/*
	* Create topic handle.
	*/
	kafka_conf->handle_topic = RdKafka::Topic::create(m_producer, kafka_conf->topic, m_tconf, err_str);
	if (!kafka_conf->handle_topic) {
		std::cerr << "Failed to create topic: " << err_str << std::endl;
		//exit(1);
		return -1;
	}

	//std::cout << "%Create topic " << kafka_conf->handle_topic->name() << std::endl;
	return 0;
}

int KafkaServer::sendMessage(KAFKA_CONF* kafka_conf, std::string& line) {
	int32_t partition = RdKafka::Topic::PARTITION_UA;
	int ret = 0;
	/*
	* Produce message
	*/
	RdKafka::ErrorCode resp = m_producer->produce(kafka_conf->handle_topic, partition,
			RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
			const_cast<char *>(line.c_str()), line.size(),
			NULL, NULL);
	if (resp != RdKafka::ERR_NO_ERROR) {
		std::cerr << "% Produce failed: " << RdKafka::err2str(resp) << std::endl;
		ret = -1;
	}

	m_producer->poll(0);

	return ret;
}

void KafkaServer::disconnectServer(KAFKA_CONF* kafka_conf) {
	run = true;
	while (run and m_producer->outq_len() > 0) {
		m_producer->poll(1000);
	}

	if (NULL != kafka_conf->handle_topic)
	{
		delete kafka_conf->handle_topic;
		kafka_conf->handle_topic = NULL;
	}
}

KafkaServer::~KafkaServer() {
	/*
	* Wait for RdKafka to decommission.
	* This is not strictly needed (when check outq_len() above), but
	* allows RdKafka to clean up all its resources before the application
	* exits so that memory profilers such as valgrind wont complain about
	* memory leaks.
	*/

	m_conf = NULL;
	m_tconf = NULL;
	delete m_producer;
	m_producer = NULL;
	err_str = "";
	RdKafka::wait_destroyed(5000);
}

void init_topic_struct(KAFKA_CONF* kafkaconf) {
	kafkaconf->handle_topic = NULL;
}
