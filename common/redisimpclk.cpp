#include <stdlib.h>
#include "redisimpclk.h"
#include <unistd.h>
#include <string.h>
#include "../common/errorcode.h"
#include "../common/setlog.h"

//#define SERVER_LOCAL "127.0.0.1"
//#define REDIS_PORT 7389

typedef struct redis_return_data
{
	sem_t extsemid;
	REDIS_INFO *extaddr;
	map<string, IMPCLKCREATIVEOBJECT> creativeinfo;
	map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK> fc;
	pthread_mutex_t g_mutex_data;
	sem_t semidarr[2];
	REDIS_INFO *addr[2];
	redisContext *g_context;
	redisContext *pubsub;
	uint64_t logid_local;
	uint8_t adx;
	pthread_t tid;
	string slave_ip;
	int slave_port;
	bool *run_flag;
}REDSI_RETURN_DATA;

template<typename T1>
void printVectorInt(vector<T1> &v)
{
	int i = 0;
	for(; i < v.size(); i++)
		cout << (int)v[i] << endl;
}

void printVectorString(vector<string> &v)
{
	int i = 0;
	for(; i < v.size(); i++)
		cout << v[i] << endl;
}

template<typename T2>
void printSetInt(set<T2> &v)
{
	typename set<T2>::iterator it;
	for(it=v.begin();it!=v.end();it++) 
		cout<<(int)*it <<endl;   
}

void printStringSet(set<string> &v)
{
	set<string>::iterator it;
	for(it=v.begin();it!=v.end();it++) 
		cout<< *it <<endl;   
}

void get_semaphore(IN uint64_t ctx, OUT uint64_t *cache, OUT sem_t *semid)
{
	REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)ctx;
	va_cout("addr is %p", redisdata);
	pthread_mutex_lock(&redisdata->g_mutex_data);
	*cache = (uint64_t)redisdata->extaddr;
	*semid = redisdata->extsemid;
	pthread_mutex_unlock(&redisdata->g_mutex_data);
}

void clearStruct(REDIS_INFO *redis_info)
{
	redis_info->creativeinfo.clear();
	redis_info->fc.clear();
}

void initIMPCLKCREATIVEOBJECT(IMPCLKCREATIVEOBJECT &mapidgroup)
{
	mapidgroup.groupid = "";
	mapidgroup.lastrefreshtime = getCurrentTimeSecond();
}

int addGroup(redisContext *g_context, uint64_t logid_local, REDIS_INFO *redis_info, string groupid)
{
	set<string> mapids;
	json_t *root, *label;
	int errorcode = E_SUCCESS;
	string dsp_mapids;
	FREQUENCYCAPPINGRESTRICTIONIMPCLK frec;

	init_FCIMPCLK_object(frec);
	errorcode = getFrequencyCappingRestrictionimpclk(g_context, logid_local, groupid.c_str(), frec);
	if(errorcode == E_SUCCESS)
	{
		redis_info->fc.insert(make_pair(groupid, frec));
	}
	else if (errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}

	root = label = NULL;
	errorcode = getRedisValue(g_context, logid_local, "dsp_groupid_mapids_" + groupid, dsp_mapids);
	if(errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto exit;
	}
	json_parse_document(&root, dsp_mapids.c_str());
	if(root == NULL)
	{
		errorcode = E_INVALID_PARAM_JSON;
		goto exit;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "mapids", JSON_ARRAY))
	{
		json_t *mapidtemp  = NULL;
		if (label->child == NULL || label->child->child == NULL)
		{
			va_cout("mapid value is NULL");
			json_free_value(&root);
			goto exit;
		}
		mapidtemp = label->child->child;//the iterator of mapid list
		while (mapidtemp != NULL)//mapid list
		{
			if((mapidtemp->type == JSON_STRING) && (strlen(mapidtemp->text) > 0))
			{
				IMPCLKCREATIVEOBJECT mapidgroup;
				initIMPCLKCREATIVEOBJECT(mapidgroup);
				mapidgroup.groupid = groupid;
				redis_info->creativeinfo.insert(make_pair(mapidtemp->text, mapidgroup));
			}
			mapidtemp = mapidtemp->next;
		}
	}
exit:
	if(root != NULL)
	{
		json_free_value(&root);
	}
	return errorcode;
}

void cacheCmpCopy(REDSI_RETURN_DATA *redisdata, int index)
{
	REDIS_INFO *redis_info = redisdata->addr[index];
	//创意所属项目信息
	map<string, IMPCLKCREATIVEOBJECT>::iterator map_it  =redisdata->creativeinfo.begin();
	while(map_it != redisdata->creativeinfo.end())
	{
		map<string, IMPCLKCREATIVEOBJECT>::iterator map_it_cache = redis_info->creativeinfo.find(map_it->first);
		//在redis_info中找到，更新cache的lastrefreshtime
		/*if(map_it_cache != redis_info->creativeinfo.end())
		{
			IMPCLKCREATIVEOBJECT &v_mapid = map_it_cache->second;
			v_mapid.lastrefreshtime = getCurrentTimeSecond();
		}
		else//未找到，查看cache的lastrefreshtime是否超过LIFEGROUP，没有就插入redis_info，否则从cache中移除
		{
			IMPCLKCREATIVEOBJECT &v_mapid = map_it->second;
			uint32_t time_now = getCurrentTimeSecond();
			if(time_now - v_mapid.lastrefreshtime < LIFEGROUP)
				redis_info->creativeinfo.insert(make_pair(map_it->first, v_mapid));
			else
				redisdata->creativeinfo.erase(map_it->first);

		}*/
		if(map_it_cache == redis_info->creativeinfo.end())
		{
			writeLog(redisdata->logid_local, LOGDEBUG, map_it->first + " not find in dsp_groupid_mapids");
			IMPCLKCREATIVEOBJECT &v_mapid = map_it->second;
			uint32_t time_now = getCurrentTimeSecond();
			if(time_now - v_mapid.lastrefreshtime < LIFEGROUP)
			{
				redis_info->creativeinfo.insert(make_pair(map_it->first, v_mapid));
			}
		}
		map_it++;
	}
	redisdata->creativeinfo.clear();
	//将redis_info中的内容拷贝到cashe中
	redisdata->creativeinfo = redis_info->creativeinfo;
	//项目频次信息
	map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK>::iterator map_it_fc = redisdata->fc.begin();
	while(map_it_fc != redisdata->fc.end())
	{
		map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK>::iterator map_it_fc_cache = redis_info->fc.find(map_it_fc->first);
		if(map_it_fc_cache == redis_info->fc.end())
		{
			writeLog(redisdata->logid_local, LOGDEBUG, map_it_fc->first + " not find in dsp_groupid_frequencycaping");
			FREQUENCYCAPPINGRESTRICTIONIMPCLK &fc = map_it_fc->second;
			uint32_t time_now = getCurrentTimeSecond();
			if(time_now - fc.lastrefreshtime < LIFEGROUP)
				redis_info->fc.insert(make_pair(map_it_fc->first, fc));
		}
		++map_it_fc;
	}
	redisdata->fc.clear();
	redisdata->fc = redis_info->fc;
	return;
}

int initializeStruct(REDSI_RETURN_DATA *redisdata, int index)
{
	clearStruct(redisdata->addr[index]);
	int errorcode = E_SUCCESS;

	string dsp_groupids = "";
	json_t *dsp_root, *dsp_label;
	uint64_t logid_local = redisdata->logid_local;
	dsp_root = dsp_label = NULL;
	errorcode = getRedisValue(redisdata->g_context, logid_local, "dsp_groupids", dsp_groupids);
	if (errorcode == E_REDIS_CONNECT_INVALID)
	{
		goto pap;
	}
	//没有在投项目可能就没有dsp_groupids的key
	if (dsp_groupids.size() == 0)
	{
		writeLog(logid_local, LOGINFO, "dsp_groupids does not exist!");
		errorcode = E_SUCCESS;
		goto pap;
	}
	//va_cout("dsp_groupids %s",  dsp_groupids.c_str());
	json_parse_document(&dsp_root, dsp_groupids.c_str());
	if (dsp_root == NULL)
    {
        va_cout("parse dsp_groupids failer");
        errorcode = E_INVALID_PARAM_JSON;
        goto pap;
    }

	if(JSON_FIND_LABEL_AND_CHECK(dsp_root, dsp_label, "groupids", JSON_ARRAY))
    {
        json_t *temp;
        temp = dsp_label->child->child;//the iterator of dsp_groupid list
        while(temp != NULL)
        {
            if((temp->type == JSON_STRING) && (strlen(temp->text) > 0))
            {
                    errorcode = addGroup(redisdata->g_context, logid_local, redisdata->addr[index], string(temp->text));
                    if(errorcode == E_REDIS_CONNECT_INVALID)
                        goto pap;
			}
            temp = temp->next;
        }
    }

pap:
    string pap_groupids = "";
    json_t *pap_root, *pap_label;
    pap_root = pap_label = NULL;

    errorcode = getRedisValue(redisdata->g_context, logid_local, "pap_groupids", pap_groupids);
    if (errorcode == E_REDIS_CONNECT_INVALID)
    {
        goto exit;
    }
    //没有在投项目可能就没有pap_groupids的key
    if (pap_groupids.size() == 0)
    {
        writeLog(logid_local, LOGINFO, "pap_groupids does not exist!");
        errorcode = E_SUCCESS;
        goto exit;
    }
    //va_cout("pap_groupids %s",  pap_groupids.c_str());
    json_parse_document(&pap_root, pap_groupids.c_str());
    if (pap_root == NULL)
    {
        va_cout("parse pap_groupids failer");
        errorcode = E_INVALID_PARAM_JSON;
        goto exit;
    }
    if (JSON_FIND_LABEL_AND_CHECK(pap_root, pap_label, "groupids", JSON_ARRAY))
    {
        json_t *temp;
        temp = pap_label->child->child; //the iterator of pap_groupid list
        while (temp != NULL)
        {
            if ((temp->type == JSON_STRING) && (strlen(temp->text) > 0))
            {
                errorcode = addGroup(redisdata->g_context, logid_local, redisdata->addr[index], string(temp->text));
                if (errorcode == E_REDIS_CONNECT_INVALID)
                    goto exit;
            }
            temp = temp->next;
        }
    }
	
exit:
	
	if(dsp_root != NULL)
        json_free_value(&dsp_root);
    if(pap_root != NULL)
                json_free_value(&pap_root);
	
	cacheCmpCopy(redisdata, index);
	return errorcode;
}

void *major_thread(void *arg)
{
	REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)arg;
	uint32_t choose = 0;
	uint32_t next = 0;
//	pthread_detach(pthread_self());
	int errcode = E_SUCCESS;
	//redisReply *reply;
	//const char* command = "PSUBSCRIBE dsp_news";  
	/*redisContext *context;
 	context = redisConnect("127.0.0.1", 8389);
	if (context->err)
	{
		redisFree(g_context);
		cout << "Execute redisConnect" << string(SERVER_LOCAL) <<"failed"<<endl;
		return NULL;
	}*/
	/*reply = (redisReply*)redisCommand(redisdata->pubsub, command);  
    if(context->err != 0)
	{
		 cout <<"execute command "<<command <<"failed" << endl;
	}
	else
		cout <<"execute subscribe success " << command << endl;
			   
	freeReplyObject(reply);
	while(redisGetReply(context, (void **)&reply) == REDIS_OK)
	*/
	while(*redisdata->run_flag)
	{
		//printf("ready to fresh\n");
		next = choose + 1;
		if (redisdata->g_context == NULL)  //reconnect
		{
		   redisdata->g_context = redisConnect(redisdata->slave_ip.c_str(), redisdata->slave_port);
		   if (redisdata->g_context && redisdata->g_context->err)
		   {
			   writeLog(redisdata->logid_local, LOGDEBUG, "cache reconnect err!");
			   redisFree(redisdata->g_context);
			   redisdata->g_context = NULL;
		   }
		}

		if (redisdata->g_context != NULL)
		{
			errcode = initializeStruct(redisdata, next %2);

			if (redisdata->adx == ADX_MAX && errcode == E_REDIS_CONNECT_INVALID)   
			{
				writeLog(redisdata->logid_local, LOGDEBUG, "initializeStruct context is invaild!");
				redisFree(redisdata->g_context);
				redisdata->g_context = NULL;
			}
		}

		pthread_mutex_lock(&redisdata->g_mutex_data);
		redisdata->extsemid = redisdata->semidarr[next %2];
		redisdata->extaddr = redisdata->addr[next%2];
		pthread_mutex_unlock(&redisdata->g_mutex_data);
		sleep(5);
		if(semaphore_test(redisdata->semidarr[choose%2]))
		{
			choose++;
			//printf("no refreance\n");
		}
		//freeReplyObject(reply);
	}
	//pthread_mutex_destroy(&redisdata->g_mutex_data);
	cout<<"cache major thread exit"<<endl;
	return NULL;
}


void myprintffc(FREQUENCYCAPPINGRESTRICTIONIMPCLK fc)
{
	printf("groupid: %s\n", fc.groupid.c_str());
	cout <<"lastrefresh time :" << fc.lastrefreshtime <<endl;
	printf("group frequency:\n");
	map<uint8_t, FREQUENCY_RESTRICTION_GROUP>::const_iterator map_it_t = fc.frg.begin();
	while(map_it_t != fc.frg.end())
	{
		FREQUENCY_RESTRICTION_GROUP frg = map_it_t->second;
		printf("adx: %d\n", map_it_t->first);
		printf("type: %u\n", frg.type);
		printf("period: %u\n", frg.period);
		printf("frequency:\n");
		printVectorInt(frg.frequency);
		++map_it_t;
	}

	printf("user frequency :\n");
	printf("type: %u\n", fc.fru.type);
	printf("period: %u\n", fc.fru.period);
	map<string, uint32_t>::const_iterator map_it = fc.fru.capping.begin();
	while(map_it != fc.fru.capping.end())
	{
		cout <<"mapid :" << map_it->first <<endl;
		cout <<"frequency :" << map_it->second <<endl;
		map_it++;
	}
	return;
}

void myprintfcrvaandgr(IMPCLKCREATIVEOBJECT creativeinfo)
{
	cout <<"groupid :" <<creativeinfo.groupid <<endl;
	cout <<"lastrefresh time :" << creativeinfo.lastrefreshtime <<endl;
	return;
} 

void parseStruct(uint64_t addr)
{
	va_cout("parseStruct\n");
	REDIS_INFO *redis =(REDIS_INFO *)addr;
	map<string, IMPCLKCREATIVEOBJECT>::iterator map_it_co = redis->creativeinfo.begin();
	while(map_it_co != redis->creativeinfo.end())
	{
		cout <<"mapid :" << map_it_co->first <<endl;
		IMPCLKCREATIVEOBJECT creativeinfo = map_it_co->second;
		myprintfcrvaandgr(creativeinfo);
		++map_it_co;
	}

	map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK>::iterator map_it_fc = redis->fc.begin();
	while(map_it_fc != redis->fc.end())
	{
		cout <<"fc" << map_it_fc->first <<endl;
		FREQUENCYCAPPINGRESTRICTIONIMPCLK fc = map_it_fc->second;
		myprintffc(fc);
		++map_it_fc;
	}

	return;
}

void *operate(void *arg)
{
	pthread_detach(pthread_self());
	uint64_t data = (uint64_t)arg;
	uint64_t addr = 0;
	sem_t sem_id;
	//sleep(20);
	//int i = 4;
	while(1)
	//while(i)
	{
		
		get_semaphore(data, &addr, &sem_id);
		printf("thid is %u semid is %d	addr is %p\n", (unsigned)pthread_self(), sem_id, addr);
		if(semaphore_release(sem_id))
		{
			printf("wait to die\n");
			sleep(100);
			parseStruct(addr);
			if(semaphore_wait(sem_id))
		//		;
				printf("wait ok\n");
		}
		//i--;
	}
	//printf("over\n");
}

int init_redis_operation(IN uint64_t logid_local, IN uint8_t adx, IN char *slave_ip, IN int slave_port, IN char *pubsub_ip, IN int pubsub_port, IN bool *run_flag, OUT vector<pthread_t> &v_thread_id, OUT uint64_t *pctx)
{
	int ret = E_SUCCESS;
	pthread_t threadid;
	REDSI_RETURN_DATA *redisdata = NULL;
	int res;
	redisdata = new REDSI_RETURN_DATA;
	if (redisdata == NULL)
	{
		ret = E_MALLOC;
		goto exit;
	}

	redisdata->g_context = NULL; 
	redisdata->slave_ip = slave_ip;  //20170708: add
	redisdata->slave_port = slave_port;  //20170708: add
	redisdata->addr[0] = redisdata->addr[1] = NULL;

	redisdata->g_context = redisConnect(slave_ip, slave_port);
	if (redisdata->g_context->err)
	{
		va_cout("Execute redisConnect %s failed", slave_ip);
		ret = E_REDIS_SLAVE_MAJOR;
		goto exit;
	}

	if(!createsemaphore(redisdata->semidarr[0]))
	{
		ret = E_SEMAPHORE;
		goto exit;
	}
	if(!createsemaphore(redisdata->semidarr[1]))
	{
		ret = E_SEMAPHORE;
		goto exit;
	}

	//cout << "semid is " << redisdata->semidarr[0] << " and " << redisdata->semidarr[1] <<endl;

	redisdata->addr[0] = new REDIS_INFO;
	if(redisdata->addr[0] == NULL)
	{
		ret = E_MALLOC;
		goto exit;
	}
	redisdata->addr[1] = new REDIS_INFO;
	if(redisdata->addr[1] == NULL)
	{
		ret = E_MALLOC;
		goto exit;
	}
	//va_cout( "addr is %p and %p" , addr[0] ,addr[1]);
	pthread_mutex_init(&redisdata->g_mutex_data, 0);
	redisdata->logid_local = logid_local;
	redisdata->adx = adx;
	ret = initializeStruct(redisdata, 0);
	if(ret == E_REDIS_CONNECT_INVALID)
		goto exit;

	//parseStruct((uint64_t)redisdata->addr[0]);
	pthread_mutex_lock(&redisdata->g_mutex_data);
	redisdata->extsemid = redisdata->semidarr[0];
	redisdata->extaddr = redisdata->addr[0];
	pthread_mutex_unlock(&redisdata->g_mutex_data);

	redisdata->run_flag = run_flag;
	res = pthread_create(&redisdata->tid, NULL, major_thread, (void *)redisdata);	
	if(res != 0)
	{
		va_cout("create thread failed\n");
		ret = E_CREATETHREAD;
		goto exit;
	}
	va_cout("initglobalData ok, will leave!");
	va_cout("addr return is %p", redisdata);
	*pctx = (uint64_t)redisdata;

exit:
	if(ret != E_SUCCESS)
	{
		if(redisdata != NULL)
		{
			if(redisdata->g_context != NULL)
			{
				redisFree(redisdata->g_context);
				redisdata->g_context = NULL;
			}
			if(redisdata->addr[0] != NULL)
			{
				delete redisdata->addr[0];
				redisdata->addr[0] = NULL;
			}
			if(redisdata->addr[1] != NULL)
			{
				delete redisdata->addr[0];
				redisdata->addr[1] = NULL;
			}
			delete redisdata;
			redisdata = NULL;
		}
	}
	return ret;
}

void uninit_redis_operation(uint64_t arg)
{
	REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)arg;
	del_semvalue(redisdata->semidarr[0]);
	del_semvalue(redisdata->semidarr[1]);
//	pthread_cancel(redisdata->tid);
//	pthread_join(redisdata->tid, NULL);
	if(redisdata->g_context != NULL)
	{
		redisFree(redisdata->g_context);
		redisdata->g_context = NULL;
	}
	//redisFree(redisdata->pubsub);
	cout <<"major thread quit" <<endl;
	/*
	{
		pthread_mutex_destroy(&redisdata->g_mutex_data);
		delete redisdata->addr[0];
		delete redisdata->addr[1];
		delete redisdata;
	}*/
	return;
}

/*int main()
{
	pthread_t threadid, threadid2;
	pthread_t thidarr[10];
	threadid = threadid2 = 0;
	int res;
	int ret;
	bool run_flag = true;
	uint64_t ctx;
	vector<pthread_t> v_thread_id;

	uint64_t logid = openLog("/etc/dspads_odin/dsp_amax.conf","locallog", true, &run_flag, v_thread_id, false);

	ret = init_redis_operation(logid, ADX_AMAX,  SERVER_LOCAL, REDIS_PORT, SERVER_LOCAL, REDIS_PORT, &run_flag, v_thread_id, &ctx);;
	if(ret == 0)
	{
		printf("parse success\n");
		//REDSI_RETURN_DATA *redisdata = (REDSI_RETURN_DATA *)ctx;
		//writeLog(logid, LOGINFO, "hello world");
		//sleep(50);
		//va_cout("semid1 is %d\n", redisdata->semidarr[0]);
		//va_cout("semid2 is %d\n", redisdata->semidarr[1]);

		res = pthread_create(&threadid2, NULL, operate, (void *)ctx);
		if(res != 0)
		{
		printf("create thread failed\n");
		}
		else
		{
			pthread_join(threadid2, NULL);
		}
	}
	return 0;
}*/
