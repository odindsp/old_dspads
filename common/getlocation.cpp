#include <syslog.h>
#include<arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>  
#include <stddef.h>  
#include <stdarg.h>  
#include <string.h>  
#include <assert.h>  
#include <fstream>
#include <time.h>
#include <iostream>
#include "errorcode.h"
#include "getlocation.h"

using namespace std;
#define  METASIZE	256

typedef struct addrinfo
{
	uint32_t b;
	uint32_t e;
	uint32_t regioncode;
}ADDRINFO;

typedef struct ipbinfo
{
	vector<ADDRINFO>addrList;
	vector<ADDRINFO>addrListIndex;
}IPBINFO;

void init_ADDRINFO(ADDRINFO &addr)
{
	addr.b = 0;
	addr.e = 0;
	addr.regioncode = 0;
}

void revertFormat(char *IPbegin, char *IPend, int regioncde, ADDRINFO &addr)
{
	int sub[4]= {0};
	sscanf(IPbegin, "%d.%d.%d.%d", &sub[3], &sub[2], &sub[1], &sub[0]);
	for(int i = 0; i < 4; i++)
		addr.b += sub[i] * (1 <<(8 *i));
	sscanf(IPend, "%d.%d.%d.%d", &sub[3], &sub[2], &sub[1], &sub[0]);
	for(int i = 0; i < 4; i++)
		addr.e += sub[i] * (1 <<(8 *i));
	//addr.regioncode = regioncde - 1156000000;
	addr.regioncode = regioncde;
	return;
}

void my_print(ADDRINFO addr)
{
	cout <<"IPbegin: " <<addr.b <<" IPend: " << addr.e<<" regioncde :" <<addr.regioncode <<endl;
}

int initInfo(IPBINFO *ipb_info, char *fname)
{
	fstream file_in(fname);
	if(file_in.is_open())
	{
		char addrorigin[4096]; 
		char IPbegin[20];
		char IPend[20];
		uint32_t regioncode;
		vector<ADDRINFO> &addrList = ipb_info->addrList;

		while(file_in.getline(addrorigin, 4096))
		{ 
			ADDRINFO addr;
			//cout <<"origin :" <<addrorigin<<endl;
			sscanf(addrorigin, "%s %s %d", IPbegin, IPend, &regioncode);
			//cout <<"IPbegin :" <<IPbegin <<endl;
			//cout <<"IPend :" <<IPend << endl;
			//cout <<"regioncde :" <<regioncode << endl;
			init_ADDRINFO(addr);
			revertFormat(IPbegin, IPend, regioncode, addr);
			ipb_info->addrList.push_back(addr);
		} 
		file_in.close();
		int  i = 0;
		for(i = 0; i < addrList.size() - METASIZE ; i += METASIZE)
		{
			ADDRINFO addr;
			init_ADDRINFO(addr);
			addr.b = addrList[i].b;
			addr.e = addrList[i + METASIZE -1].e;
			ipb_info->addrListIndex.push_back(addr);
		}

		if(i >= addrList.size() - METASIZE)
		{
			ADDRINFO addr;
			init_ADDRINFO(addr);
			addr.b = addrList[i].b;
			addr.e = addrList[addrList.size() - 1].e;
			ipb_info->addrListIndex.push_back(addr);
		}
	}
	else
	{
		return E_FILE;
	}
	return E_SUCCESS;
}

int getRegionCode(uint64_t addr, string &IPstr)
{
	IPBINFO *ipb_info = (IPBINFO *)addr;
	vector<ADDRINFO> &addrList = ipb_info->addrList;
	vector<ADDRINFO> &addrListIndex = ipb_info->addrListIndex;
	int sub[4]= {0};
	if(IPstr.find(" ") != string::npos)
	{
		trimstring(IPstr);
	}

	uint32_t IPint = inet_addr(IPstr.c_str());;
	if(IPint == INADDR_NONE)
	    return 0;
	else
		IPint = ntohl(IPint);

	int index = -1;
	int64_t low, high, mid;
	{
		low = 0;
		high = addrListIndex.size();
		while(low  <= high)
		{
			mid = ((low + high) >> 1);
			if(IPint <= addrListIndex[mid].e && IPint>= addrListIndex[mid].b)
			{
				index = mid;
				break;
			}
			else if(IPint < addrListIndex[mid].b)
			{
				high = mid -1;
				if(low <= high && IPint > addrListIndex[high].e)
					break;
			}
			else if(IPint > addrListIndex[mid].e)
			{
				low = mid + 1;
				if(low <= high && IPint < addrListIndex[low].b)
					break;
			}
			else
			{
				cout <<"not belong to china" <<endl;
				break;
			}
		}
	}
	if(index != -1)
	{
		low = index * METASIZE;
		if(index == addrListIndex.size() -1)
			high = addrList.size();
		else
			high = low + METASIZE;
		while(low  <= high)
		{
			mid = ((low + high) >> 1);
			if(IPint <= addrList[mid].e && IPint>= addrList[mid].b)
			{
				return addrList[mid].regioncode;
			}
			else if(IPint < addrList[mid].b)
			{
				high = mid -1;
				if(low <= high && IPint > addrList[high].e)
					return 0;
			}
			else if(IPint > addrList[mid].e)
			{
				low = mid + 1;
				if(low <= high && IPint < addrList[low].b)
					return 0;
			}
			else
			{
				cout <<"not belong to china" <<endl;
				return 0;
			}
		}
	}
	return 0;
}

uint64_t openIPB(char *fname)
{
	IPBINFO *ipb_info = new IPBINFO;
	int errno = initInfo(ipb_info, fname);
	if(errno == E_SUCCESS)
		return (uint64_t)ipb_info;
	else
	{
		delete ipb_info;
		return 0;
	}
}

void closeIPB(uint64_t addr)
{
	IPBINFO *ipb_info = (IPBINFO *)addr;
	delete ipb_info;
}

/*int main(int argc, char *argv[])
{
	uint64_t mmdb;

	char IPstr[20] = "36.110.73.210";
	if(argc > 1)
		strcpy(IPstr, argv[1]);
	mmdb = openIPB("/etc/dspads_odin/ipb");
	if(mmdb)
	{

		int ret = getRegionCode(mmdb, IPstr);
		printf("regionCode is %d\n",ret);


		closeIPB(mmdb);
	}
}*/
