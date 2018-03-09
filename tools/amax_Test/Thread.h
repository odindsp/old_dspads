#ifndef THREAD_H
#define THREAD_H
#include <QThread>
#include <QNetworkAccessManager>
#include <QUrl>

class Thread : public QThread
{
    Q_OBJECT
public:
    //Thread(unsigned int numRequest);
    explicit Thread(QThread *parent = 0);
    void set(unsigned int num);
    virtual void run();
    ~Thread();
    void _slotManagerFinished(QNetworkReply  * reply);
public:
    unsigned int numRequest;        //每个线程发送请求数
    //QNetworkAccessManager *nam ;
};

#endif // THREAD_H
