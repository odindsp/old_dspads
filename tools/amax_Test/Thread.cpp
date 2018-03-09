#include "Thread.h"
#include "Amax_Test.h"
#include "ui_Amax_Test.h"
#include "common.h"
#include <QMutex>

/*Thread::Thread(unsigned int numRequest)
{
    this->numRequest = numRequest;
}
*/
void Thread::set(unsigned int num)
{
    this->numRequest = num;
}

Thread::Thread(QThread *parent) :QThread(parent)
{

}

Thread::~Thread()
{

}
QUrl url("http://192.168.3.62/pv?id=12345678911&pv=1&uv=0&pu=http://www.baidu.com&sys=3&ds=1920*1200&bid=213456&mid=123456&rnd=1232037192&ty=1&cl=http://www.baidu.com&mb=xiaomi");
void Thread::run()
{
    //qDebug()<<this->currentThreadId()<<endl;
    //QMap<QString, QString>g_m;
    QNetworkRequest req;
    QNetworkAccessManager nam ;
    req.setUrl(url);
    //for(unsigned int i = 0 ; i < this->numRequest ;++i )
    //{
        do_get(nam, req, this->numRequest);
    //}
}

