/*
 * tcp.h
 *
 *  Created on: Nov 4, 2014
 *      Author: user
 */

#ifndef TCP_H_
#define TCP_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include "tcp.h"

typedef struct tcp_header TCPHEADER;
struct tcp_header
{
	char *name;
	char *value;
	TCPHEADER *next; /* FIXME: this is ugly; need cons cell. */
};

struct tcp_request
{
	char *uri;
	int majorversion;
	int minorversion;
	TCPHEADER *header;
	char *recvmsg;
};
typedef struct tcp_request TCPREQUEST;

struct tcp_response
{
	int majorversion;
	int minorversion;
	int statuscode;
	char *statusmessage;
	TCPHEADER *header;
};
typedef struct tcp_response TCPRESPONSE;

#ifdef __cplusplus
extern "C" {
#endif

uint64_t ntohl64(uint64_t arg64);
uint64_t htonl64(uint64_t arg64);
int connectServer(const char *address, const int port);
int disconnectServer(int fd);
int receiveMessage(int fd, char **buffer, uint32_t *length);
int sendMessage(int fd, const char *message, const uint32_t length);

#ifdef __cplusplus
}
#endif


#endif /* TCP_H_ */
