#ifndef MYTHREAD_H
#define MYTHREAD_H
#include <QMutex>
#include <QThread>
#include "Amax_Test.h"

class MyThread : public QThread
{
    Q_OBJECT
public:
    MyThread();
    ~MyThread();
    void run();
public:
    Ui::Amax_Test* amax_test;
    QMutex file_mutex;
    unsigned int threadNum;
    MyThread* pmythread;

};

#endif // MYTHREAD_H
