#include <sys/sem.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <stdint.h>
#include "util.h"
#include "selfsemphore.h"

using namespace std;

bool createsemaphore(sem_t &sem)
{
	int ret = sem_init(&sem, 0, 0);
	if(ret == -1)
	{
		va_cout("create semaphore failed  %s", strerror(errno));
		return false;
	}
	return true;
}

bool semaphore_test(IN sem_t &sem)
{
	int val;
	while(1)
	{
		if(sem_getvalue(&sem, &val) == 0)
		{
			if(val ==0)
				break;
		}
		else
		{
			va_cout("test semaphore failed  %s", strerror(errno));
			return false;
		}
	}
	return true;
}

bool semaphore_wait(IN sem_t &sem)
{
	va_cout("enter wait");
	/*int val;
	if(sem_getvalue(&sem, &val) == 0)
	{
		cout <<"val:" <<val <<endl;
	}*/
	if(sem_wait(&sem) == -1)
	{
		va_cout("wait semaphore failed  %s", strerror(errno));
		return false;
	}
	else
		va_cout("wait success");
	return true;
}

bool semaphore_release(IN sem_t &sem)
{
	if(sem_post(&sem) == -1)
	{
		va_cout("release semaphore failed  %s", strerror(errno));
		return false;
	}
	else
		va_cout("release success");
	/*int val;
	if(sem_getvalue(&sem, &val) == 0)
	{
		cout <<"val:" <<val <<endl;
	}*/
	return true;
}

bool del_semvalue(IN sem_t &sem)
{
	if(sem_destroy(&sem) == -1)
	{
		va_cout("del semaphore failed  %s", strerror(errno));
		return false;
	}
	return true;
}

