#ifndef SELFSEMPHORE_H_
#define SELFSEMPHORE_H_
#define IN
#define OUT

#include <semaphore.h>

union semun {
	int              val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
};

bool createsemaphore(IN sem_t &sem);
bool semaphore_test(IN sem_t &sem);
bool semaphore_wait(IN sem_t &sem);
bool semaphore_release(IN sem_t &sem);
bool del_semvalue(IN sem_t &sem);
#endif