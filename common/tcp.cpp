/*
 * tcp.pp
 *
 *  Created on: Nov 4, 2014
 *      Author: user
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <iostream>
#include "tcp.h"
using namespace std;

#define TIME_OUT_TIME 200000

uint64_t ntohl64(uint64_t arg64)
{
	uint64_t res64;
#if __BYTE_ORDER==__LITTLE_ENDIAN
	uint32_t low = (uint32_t)(arg64 & 0x00000000FFFFFFFFLL);
	uint32_t high = (uint32_t)((arg64 & 0xFFFFFFFF00000000LL) >> 32);
	low = ntohl(low);
	high = ntohl(high);
	res64 = (uint64_t)high + (((uint64_t)low) << 32);
#else
	res64 = arg64;
#endif
	return res64;
}

uint64_t htonl64(uint64_t arg64)
{
	uint64_t res64;
#if __BYTE_ORDER==__LITTLE_ENDIAN
	uint32_t low = (uint32_t)(arg64 & 0x00000000FFFFFFFFLL);
	uint32_t high = (uint32_t)((arg64 & 0xFFFFFFFF00000000LL) >> 32);
	low = htonl(low);
	high = htonl(high);
	res64 = (uint64_t)high + (((uint64_t)low) << 32);
#else
	res64 = arg64;
#endif
	return res64;
}

static int tcpErrorToErrno (int err)
{
	/* Error codes taken from RFC2068. */
	switch (err)
	{
		case -1:    /* system error */
			return errno;
		case -200:  /* OK */
		case -201:  /* Created */
		case -202:  /* Accepted */
		case -203:  /* Non-Authoritative Information */
		case -204:  /* No Content */
		case -205:  /* Reset Content */
		case -206:  /* Partial Content */
			return 0;
		case -400:  /* Bad Request */
			// printf("http_error_to_errno: 400 bad request");
			return EIO;
		case -401:  /* Unauthorized */
			// printf("http_error_to_errno: 401 unauthorized");
			return EACCES;
		case -403:  /* Forbidden */
			// printf("http_error_to_errno: 403 forbidden");
			return EACCES;
		case -404:  /* Not Found */
			// printf("http_error_to_errno: 404 not found");
			return ENOENT;
		case -411:  /* Length Required */
			// printf("http_error_to_errno: 411 length required");
			return EIO;
		case -413:  /* Request Entity Too Large */
			// printf("http_error_to_errno: 413 request entity too large");
			return EIO;
		case -505:  /* HTTP Version Not Supported       */
			// printf("http_error_to_errno: 413 HTTP version not supported");
			return EIO;
		case -100:  /* Continue */
		case -101:  /* Switching Protocols */
		case -300:  /* Multiple Choices */
		case -301:  /* Moved Permanently */
		case -302:  /* Moved Temporarily */
		case -303:  /* See Other */
		case -304:  /* Not Modified */
		case -305:  /* Use Proxy */
		case -402:  /* Payment Required */
		case -405:  /* Method Not Allowed */
		case -406:  /* Not Acceptable */
		case -407:  /* Proxy Autentication Required */
		case -408:  /* Request Timeout */
		case -409:  /* Conflict */
		case -410:  /* Gone */
		case -412:  /* Precondition Failed */
		case -414:  /* Request-URI Too Long */
		case -415:  /* Unsupported Media Type */
		case -500:  /* Internal Server Error */
		case -501:  /* Not Implemented */
		case -502:  /* Bad Gateway */
		case -503:  /* Service Unavailable */
		case -504:  /* Gateway Timeout */
			// printf("http_error_to_errno: HTTP error %d", err);
			return EIO;
		default:
			// printf("http_error_to_errno: unknown error %d", err);
			return EIO;
	}
}

static long readAll(int fd, char *buf, size_t len)
{
	size_t n, m, r;
	char *rbuf = buf;
#ifndef _WIN32
	fd_set fdsr;
	int ret;
#endif

	r = len;
	for (n = 0; n < len; n += m)
	{
#ifndef _WIN32
		FD_ZERO(&fdsr);
		FD_SET(fd, &fdsr);
		ret = select(fd + 1, &fdsr, NULL, NULL, NULL);
		if (ret == -1)
			return -1;
#endif
		m = recv(fd, rbuf + n, len - n, 0);

		if (m == 0)
		{
			r = 0;
			break;
		}
		else if (m == -1)
		{
			if (errno != EAGAIN)
			{
				r = -1;
				break;
			}
			else
				m = 0;
		}
	}

	return r;
}

static long readUntil(int fd, char ch, char **data)
{
	char *buf, *buf2;
	long n, len, bufsize;
	bool firstBackslashR = false;
	bool secondBackslashN = false;
	char expectNextChar = '\r';

	*data = NULL;

	bufsize = 100;
	buf = (char *)calloc(1, bufsize);
	if (buf == NULL)
	{
		cout << "readUntil: out of memory" << endl;
		return -1;
	}

	len = 0;
	while ((n = readAll(fd, buf + len, 1)) == 1)
	{
		if (ch == 0)
			len++;
		else if (buf[len++] == ch)
			break;

		if (len + 1 == bufsize)
		{
			bufsize *= 2;
			buf2 = (char *)realloc(buf, bufsize);
			if (buf2 == NULL)
			{
				cout << "Read_until: realloc failed" << endl;
				free(buf);
				return -1;
			}

			buf = buf2;
		}

		if (ch == 0)
		{
			if (firstBackslashR && buf[len] != expectNextChar)
				firstBackslashR = false;
			if (buf[len - 1] != expectNextChar)
				continue;
			if (expectNextChar == '\r')
			{
				firstBackslashR = true;
				expectNextChar = '\n';
				continue;
			}
			if (expectNextChar == '\n')
			{
				if (!secondBackslashN)
				{
					expectNextChar = '\r';
					secondBackslashN = true;
					continue;
				}
				break;
			}
		}
	}

	if (n <= 0)
	{
		free(buf);
		if (n == 0)
			cout << "readUntil: closed" << endl;
		else
			cout << "readUntil: read error: " << strerror(errno) << endl;
		return n;
	}

	/* Shrink to minimum size + 1 in case someone wants to add a NUL. */
	buf2 = (char *)realloc(buf, len + 1);
	if (buf2 == NULL)
		cout << "readUntil: realloc: shrink failed" << endl; /* not fatal */
	else
		buf = buf2;

	*data = buf;
	return len;
}

static long tcpParseHeader(int fd, TCPHEADER **header)
{
	char buf[2];
	char *data;
	TCPHEADER *h;
	size_t len;
	long n;
	static int i = 0;

	*header = NULL;
	n = readAll(fd, buf, 2);
	if (n <= 0)
		return n;
	if (buf[0] == '\r' && buf[1] == '\n')
		return n;

	h = (TCPHEADER *)calloc(1, sizeof(TCPHEADER));
	if (h == NULL)
	{
		cout << "tcpParseHeader: malloc failed." << endl;
		return -1;
	}
	*header = h;
	h->name = NULL;
	h->value = NULL;

	n = readUntil(fd, ':', &data);
	if (n <= 0)
	{
		free(data);
		return n;
	}
	data = (char *)realloc(data, n + 2);
	if (data == NULL)
	{
		cout << "tcpParseHeader: realloc failed." << endl;
		return -1;
	}
	memmove(data + 2, data, n);
	memcpy(data, buf, 2);
	n += 2;
	data[n - 1] = 0;
	h->name = strdup(data);
	free(data);
	len = n;
	n = readUntil(fd, '\r', &data);

	if (n <= 0)
	{
		free(data);
		return n;
	}
	data[n - 1] = '\0';
	h->value = (char *)calloc(1, n);
	memcpy(h->value, data, n);
	free(data);
	len += n;

	n = readUntil(fd, '\n', &data);
	if (n <= 0)
	{
		free(data);
		return len;
	}
	free(data);
	if (n != 1)
	{
		cout << "tcpParseHeader: invalid line ending" << endl;
		return -1;
	}
	len += n;

	n = tcpParseHeader(fd, &h->next);
	if (n <= 0)
		return n;
	len += n;

	return len;
}

static void tcpDestroyHeader(TCPHEADER *header)
{
	if (header == NULL)
		return;

	tcpDestroyHeader(header->next);

	if (header->name)
		free(header->name);
	if (header->value)
		free(header->value);
	free(header);
}

static void tcpDestroyRequest(TCPREQUEST *request)
{
	if (request == NULL)
		return;

	if (request->uri)
		free(request->uri);
	tcpDestroyHeader(request->header);
	free(request);
}

static int tcpTransmitMessage(int fd, void *buff, int bufflen)
{
	int total = 0;

	if (bufflen == 0)
		return 0;

	do
	{
		int nbytes;

		nbytes = recv(fd, (char *)buff, bufflen, 0);

		if (nbytes <= 0)
			return -1;

		buff = (void *)((char *)buff + nbytes);
		bufflen -= nbytes;
		total += nbytes;
	} while (bufflen > 0);

	return total;
}

static long tcpParseRequest(int fd, TCPREQUEST **request_)
{
	TCPREQUEST *request;
	char *data;
	size_t len;
	long n;

	*request_ = NULL;

	request = (TCPREQUEST *)calloc(1, sizeof(TCPREQUEST));
	if (request == NULL) {
		cout << "tcpParseRequest: out of memory!" << endl;
		return -1;
	}

	request->uri = NULL;
	request->majorversion = -1;
	request->minorversion = -1;
	request->header = NULL;

	n = readUntil(fd, '/', &data);
	if (n <= 0)
	{
		free(data);
		tcpDestroyRequest(request);
		return n;
	}
	else if (n != 5 || memcmp(data, "HTTP", 4) != 0)
	{
		cout << "tcpParseRequest: expected \"HTTP\"" << endl;
		free(data);
		tcpDestroyRequest(request);
		return -1;
	}
	free(data);
	len += n;

	n = readUntil(fd, '.', &data);
	if (n <= 0)
	{
		free(data);
		tcpDestroyRequest(request);
		return n;
	}
	data[n - 1] = 0;
	request->majorversion = atoi(data);
	free(data);
	len += n;

	n = readUntil(fd, '\r', &data);
	if (n <= 0)
	{
		free(data);
		tcpDestroyRequest(request);
		return n;
	}
	data[n - 1] = 0;
	request->minorversion = atoi(data);
	free(data);
	len += n;

	n = readUntil(fd, '\n', &data);
	if (n <= 0)
	{
		free(data);
		tcpDestroyRequest(request);
		return n;
	}
	free(data);
	if (n != 1)
	{
		cout << "tcpParseRequest: invalid line ending" << endl;
		tcpDestroyRequest(request);
		return -1;
	}
	len += n;

	n = tcpParseHeader(fd, &request->header);
	if (n <= 0)
	{
		tcpDestroyRequest(request);
		return n;
	}
	len += n;

	*request_ = request;

	return len;
}

static int tcpVarifyRequest(int errorcode, TCPREQUEST *request)
{
	if (errorcode <= 0)
	{
		if (errorcode == 0)
			errorcode = -1;
		else
			cout << "tcpVarifyRequest: no request; error: " << strerror(errno) << endl;
	}
	else if (request->majorversion != 1 || (request->minorversion != 1
				&& request->minorversion != 0))
	{
		char version[10];

		sprintf(version, "%d.%d", request->majorversion, request->minorversion);
		cout << "tcpVarifyRequest: unknown HTTP version: " << version << endl;
		errorcode = -1;
	}

	return errorcode;
}

int receiveMessage(int fd, char **buffer, uint32_t *bufferlength)
{
	TCPREQUEST *request;
	int errorcode;

	errorcode = tcpParseRequest(fd, &request);
	errorcode = tcpVarifyRequest(errorcode, request);
	if (errorcode > 0)
	{
		TCPHEADER *header = request->header;

		if (header != NULL && header->name != NULL && header->value != NULL)
		{
                        char *data = NULL;
                        int length;

                        while (1)
                        {
                                if (header == NULL || strcmp(header->name, "Content-Length") == 0)
                                {
                                        if (header == NULL)
                                        {
                                                length = readUntil(fd, 0, &data);
                                                errorcode = length;
                                        }
                                        else
                                        {
                                                length = atoi(header->value);
                                                data = (char *)malloc(length + 1);
                                                errorcode = readAll(fd, data, length);
                                        }
                                        if (errorcode <= 0)
                                        {
                                                if (data != NULL)
                                                        free(data);
                                                return errorcode;
                                        }
                                        *bufferlength = length;
                                        *buffer = (char *)malloc(sizeof(char *) * length);
                                        memcpy(*buffer, data, length);
                                        free(data);
                                        break;
                                }

				header = header->next;
			}
		}
	}

	tcpDestroyRequest(request);

	return errorcode;
}

int sendMessage(int fd, const char *message, const uint32_t length)
{
	int total = 0;
	int len = length;

	if (length == 0)
		return 0;

	do
	{
		int nbytes;

		nbytes = send(fd, message, len, 0);
		if (nbytes <= 0)
			return -1;

		message = message + nbytes;
		len -= nbytes;
		total += nbytes;
	} while (len > 0);

	return total;
}

int connectServer(const char *address, const int port)
{
	int fd;
	struct sockaddr_in s_add;
	struct timeval tm;
	fd_set set;
	unsigned long ul = 1;
	int err;
	int ret = 0, len = sizeof(int);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
		return -1;

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr(address);
	s_add.sin_port=htons(port);

	ioctl(fd, FIONBIO, &ul);

	if(connect(fd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)) == -1)
	{
		tm.tv_sec = 0;
		tm.tv_usec = TIME_OUT_TIME;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		if (select(fd + 1, NULL, &set, NULL, &tm) > 0)
		{
			getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
			if (err == 0)
				ret = 1;
			else
				ret = 0;
		}
		else
			ret = 0;
	}
	else
		ret = 1;
	ul = 0;
	ioctl(fd, FIONBIO, &ul);

	if (!ret)
	{
		close(fd);
		return -1;
	}

	return fd;
}

int disconnectServer(int fd)
{
	close(fd);
}
