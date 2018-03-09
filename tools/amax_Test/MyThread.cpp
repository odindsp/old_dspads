#include "MyThread.h"
#include "Amax_Test.h"


MyThread::MyThread()
{
    pmythread = new MyThread[threadNum];
    amax_test = new Ui::Amax_Test();
}

MyThread::~MyThread()
{

}

void MyThread::run()
{
    QTextStream *inTextStream = amax_test->g_inTextStream ;
    QTextStream *outTextStream = amax_test->g_outTextStream ;
    for(int i = 1; ! (inTextStream.atEnd()); ++i)
    {
         QString readLineRet = inTextStream->readLine();            //每次读一个json请求
         readLineRet.remove(0,26);                                                    //去掉json前面的时间等内容
         QMap<QString,QString> m = amax_test->do_post(amax_test->getUrl(),readLineRet);
         if(m.value("StatusCode").toInt() != 200)
             (*outTextStream)<<m.value("StatusCode")<<"    "<<m.value("Result")<<"\n";
         else
             (*outTextStream)<<m.value("adxResponse")<<"\n";
          outTextStream->flush();
    }
}

