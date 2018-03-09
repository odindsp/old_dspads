#include "common.h"
#include "type.h"
#include "Thread.h"

#include "Amax_Test.h"
#include "ui_Amax_Test.h"
#include "CopyRequestByPatch_Dialog.h"
#include "ui_CopyRequestByPatch_Dialog.h"
#include "SendRequestByParallel_Dialog.h"
#include "ui_SendRequestByParallel_Dialog.h"
#include "SendRequestByPatch_Dialog.h"
#include "ui_SendRequestByPatch_Dialog.h"

#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QUrl>
#include <QThread>

Amax_Test::Amax_Test(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Amax_Test)
{
    ui->setupUi(this);
    ui->requestTextEdit->installEventFilter(this);
    ui->urlLineEdit->installEventFilter(this);      //在窗体上为adxRequestTextEdit安装过滤器
    itemDelegate = new QStyledItemDelegate();
    ui->platComboBox->setItemDelegate(itemDelegate);
    ui->methodComboBox->setItemDelegate(itemDelegate);

    //V1.0中先把压力测试部分不开放
    //ui->testByStressPushButton->setDisabled(true);

    QObject::connect(ui->platComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(setDataTypeSlot()));       //切换广告平台
    QObject::connect(ui->readRequestPushButton,SIGNAL(clicked()),this,SLOT(readRequestSlot())); //读取json文本文件
    QObject::connect(ui->sendRequestByBatchPushButton,SIGNAL(clicked()),this,SLOT(sendRequestByBatchSlot()));  //读取保存日志
    QObject::connect(ui->resetPushButton,SIGNAL(clicked()),this,SLOT(resetSlot()));     //清空
    QObject::connect(ui->sendRequestPushButton,SIGNAL(clicked()),this,SLOT(sendRequestSlot()));   //请求
    QObject::connect(ui->methodComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(setSubmitMethodSlot()));
    QObject::connect(ui->writeResponsePushButton,SIGNAL(clicked()),this,SLOT(writeResponseSlot())); //保存应答
    QObject::connect(ui->testByStressPushButton,SIGNAL(clicked()),this,SLOT(testByStressSlot())); //压力测试
    QObject::connect(ui->copyDataPushButton,SIGNAL(clicked()),this,SLOT(copyDataSlot())); //复制数据
}

Amax_Test::~Amax_Test()
{
    delete ui;
    IFREE(itemDelegate);
}

void Amax_Test::copyDataSlot()
{
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::yellow);
    QString json = ui->requestTextEdit->toPlainText();
    if(json.trimmed() == "")                                                //请求的json为空
    {
        ui->requestTextEdit->setPalette(palette);
        if(json.contains(EMPTY_STRING_JSON))
            return;
        ui->requestTextEdit->setText(EMPTY_STRING_JSON);
            return;
    }

    if( ! isJson(json))                                                         //请求的json格式不正确
    {
        ui->requestTextEdit->setPalette(palette);
        if(json.contains(ERROR_STRING_JSON))
            return;
        ui->requestTextEdit->setText(ERROR_STRING_JSON);
            return;
    }
    QFile *outFile = NULL;
    QTextStream *out = NULL;
    CopyRequestByPatch_Dialog crbp;
    if(crbp.exec() == QDialog::Accepted)
    {
        if(writeFile(this, "Log files(*.log)", QDir::currentPath(), QIODevice::WriteOnly, outFile, out))
        {
            //20160419,02:00:05.583,INF,
            QString dateTime = QDateTime::currentDateTime().toString("yyyyMMdd,hh:mm:ss.");       //获取时间
            QTime time = QTime::currentTime();
            QString msec = QString::number(time.msec());
            QString request = dateTime+msec+",INF,"+json+"\n";
            ui->sendByBatchProgressBar->setRange(0, crbp.numRequest);
            unsigned int i = 0;
            for( ; i < crbp.numRequest; ++i)
            {
                 (*out)<<request;
                 out->flush();
                ui->sendByBatchProgressBar->setValue(i);
            }
            ui->sendByBatchProgressBar->setValue(crbp.numRequest);
            if(i == crbp.numRequest)
            {
                QString count;
                QString text = "批量复制数据已完成：\n请求数："+count.setNum(i)+"条";
                QMessageBox message(QMessageBox::Information, "批量复制请求完成情况", text, QMessageBox::Ok, NULL);
                message.exec();
            }
        }
    }
    else
    {
        //点击取消的情况
        goto exit;
    }

exit:
    if(out != NULL)
    {
        delete out;
    }
    if(outFile != NULL)
    {
        outFile->close();
        delete outFile;
    }
}

bool Amax_Test::eventFilter(QObject *watched, QEvent *event)
{
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::white);
    if(watched == ui->requestTextEdit)
    {
        QString request = ui->requestTextEdit->toPlainText();
        if(event->type() == QEvent::FocusIn)    //获得焦点
        {
            ui->requestTextEdit->setPalette(palette);
            ui->requestTextEdit->setText(request.remove(EMPTY_STRING_JSON));
            ui->requestTextEdit->setText(request.remove(ERROR_STRING_JSON));
        }
    }
    if(watched == ui->urlLineEdit)
    {
        QString url = ui->urlLineEdit->text();
        if(event->type() == QEvent::FocusIn)
        {
            ui->urlLineEdit->setPalette(palette);
            ui->urlLineEdit->setText(url.remove(EMPTY_STRING_URL));
            ui->urlLineEdit->setText(url.remove(ERROR_STRING_URL));
        }
    }
    return QWidget::eventFilter(watched,event);     //最后将事件交给上层对话框
}

void Amax_Test::sendRequestByBatchSlot()
{
    int count = 0;
    QUrl url = ui->urlLineEdit->text();
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::yellow);
    if(url.isEmpty() || url.toString().trimmed() == "")             //url为空
    {
        ui->urlLineEdit->setPalette(palette);
        if(url.toString().contains(EMPTY_STRING_URL))
            return;
        ui->urlLineEdit->setText(EMPTY_STRING_URL);
            return;
    }
    if( ! url.isValid() || url.toString().contains(ERROR_STRING_URL) || url.toString().contains(EMPTY_STRING_URL))        //url格式无效
    {
        ui->urlLineEdit->setPalette(palette);
        if(url.toString().contains(ERROR_STRING_URL))            //url中不包含错误提示信息
            return;
        ui->urlLineEdit->setText(ERROR_STRING_URL);
            return;
    }

    QNetworkRequest nr;
    QMap<QString,QString> m;
    setPlatformHeader(nr);
    QTextStream *in = NULL, *out = NULL;
    QFile *inFile = NULL, *outFile = NULL;
    if(readFile(this,"Log files(*.log)", QDir::currentPath(), QIODevice::ReadOnly, inFile, in))      //打开日志文件
    {
        if( ! ui->autoSaveResponseCheckBox->isChecked() )                              //不勾选自动保存
        {
            if(writeFile(this, "Txt files(*.txt)", QDir::currentPath(), QIODevice::WriteOnly, outFile, out))      //保存响应文件
            {
            }
            else
                goto exit;     //保存文件点击取消操作
        }
        else
        {
            QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss_");       //获取时间
            outFile = new QFile(dateTime+"Response_"+ui->platComboBox->currentText()+".txt");       //拼接文本名
            if ( ! outFile->open(QIODevice::WriteOnly | QIODevice::Text))                                 //打开不成功
                return ;
            out = new QTextStream(outFile);
        }
        while( ! (in->atEnd()))
         {
             in->readLine();
             ++count;
         }
        in->seek(0);
        ui->sendByBatchProgressBar->setRange(0,count);
        for(int i = 0; ! (in->atEnd()); ++i)
        {
             QString readLineRet = in->readLine();            //每次读一个json请求
             readLineRet.remove(0,26);                                                    //去掉json前面的时间等内容
             if(do_post(ui->urlLineEdit->text(), readLineRet, nr, m))
             {
                 if(ui->autoFormatJsonCheckBox->isChecked() && m.value("StatusCode").toInt() == 200)
                 {
                     (*out)<<m.value("StatusCode")<<"    "<<m.value("Result")<<"\n";
                     QString format_response = formatText(m.value("adxResponse"));
                     (*out)<<format_response<<"\n";
                 }
                 else
                 {
                     (*out)<<m.value("StatusCode")<<"    "<<m.value("Result")<<"\n";
                     if(m.contains("adxResponse"))
                     {
                         (*out)<<m.value("adxResponse")<<"\n";
                     }
                 }
                out->flush();
               }
               else
                 goto exit;
             ui->sendByBatchProgressBar->setValue(i);
        }
        ui->sendByBatchProgressBar->setValue(count);
        if(in->atEnd())
        {
            QString countNum;
            QString text = "批量发送请求已完成：\n广告平台："+ui->platComboBox->currentText()+"\n"+"服务器地址："+url.toString()+"\n"+"请求数："+countNum.setNum(count)+"条";
            QMessageBox message(QMessageBox::Information, "批量发送请求完成情况", text, QMessageBox::Ok, NULL, Qt::Dialog);
            message.exec();
        }
    }

exit:
    if(in != NULL)
    {
         delete in;
    }
    if(inFile != NULL)
    {
         inFile->close();
         delete inFile;
    }
    if(out != NULL)
    {
         delete out;
    }
    if(outFile != NULL)
    {
         outFile->close();
         delete outFile;
    }
}

void Amax_Test::readRequestSlot()
{
    QFile *inFile = NULL;
    QTextStream *in = NULL;
    if(readFile(this, "Text files(*.txt)", QDir::currentPath(), QIODevice::ReadOnly, inFile, in))
    {
        QString  json = in->readAll();
        if(ui->autoFormatJsonCheckBox->isChecked())
        {
            ui->requestTextEdit->setText(formatText(json));
        }
        else
        {
            ui->requestTextEdit->setText(json);
        }
    }
    if(in != NULL)
    {
        delete in;
    }
    if(inFile != NULL)
    {
        inFile->close();
        delete inFile;
    }
}

void Amax_Test::resetSlot()
{
    ui->requestTextEdit->clear();
    ui->responseTextEdit->clear();
}

void Amax_Test::sendRequestSlot()
{
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::yellow);
    QString json = ui->requestTextEdit->toPlainText();
    int index = ui->methodComboBox->currentIndex();
    QUrl url = ui->urlLineEdit->text();
    if(url.isEmpty() || url.toString().trimmed() == "")             //url为空
    {
        ui->urlLineEdit->setPalette(palette);
        if(url.toString().contains(EMPTY_STRING_URL))
            return;
        ui->urlLineEdit->setText(EMPTY_STRING_URL);
            return;
    }
    if( ! url.isValid())        //url格式无效
    {
        ui->urlLineEdit->setPalette(palette);
        if(url.toString().contains(ERROR_STRING_URL))            //url中不包含错误提示信息
            return;
        ui->urlLineEdit->setText(ERROR_STRING_URL);
            return;
    }

    if(json.trimmed() == "")                                                //请求的json为空
    {
        ui->requestTextEdit->setPalette(palette);
        if(json.contains(EMPTY_STRING_JSON))
            return;
        ui->requestTextEdit->setText(EMPTY_STRING_JSON);
            return;
    }

    if( ! isJson(json))                                                         //请求的json格式不正确
    {
        ui->requestTextEdit->setPalette(palette);
        if(json.contains(ERROR_STRING_JSON))
            return;
        ui->requestTextEdit->setText(ERROR_STRING_JSON);
            return;
    }

    QMap<QString, QString> m;
    QNetworkRequest nr;
    setPlatformHeader(nr);
    if(index == 0)
         do_post(url, json, nr, m);
    else
         //do_get(url,nr,m);

    if(ui->autoFormatJsonCheckBox->isChecked())
    {
        ui->requestTextEdit->setText(formatText(json));
        if(m.value("StatusCode").toInt() != 200)
            ui->responseTextEdit->setText(m.value("StatusCode")+"    "+m.value("Result"));
        else
            ui->responseTextEdit->setText(formatText(m.value("adxResponse").toLatin1()));
    }
    else
    {
        if(m.value("StatusCode").toInt() != 200)
            ui->responseTextEdit->setText(m.value("StatusCode")+"    "+m.value("Result"));
        else
            ui->responseTextEdit->setText(m.value("adxResponse").toLatin1());
    }
}

void Amax_Test::setPlatformHeader(QNetworkRequest& nr)
{
    //给提交的Json添加头属性
    nr.setHeader(QNetworkRequest::ContentTypeHeader,HEADER_JSON);
    int index = ui->platComboBox->currentIndex();
    switch(index)
    {
    case 0:
        nr.setRawHeader(QByteArray("x-amaxrtb-version"),QByteArray("3.0"));     //"芒果移动广告交易平台(AMAX)"
        break;
    case 1:
        nr.setRawHeader(QByteArray("x-openrtb-version"),QByteArray("2.0"));     //"InMobi移动广告公司(InMobi)"
        break;
    case 2:
        break;                                   //"乐视视频移动广告平台(Letv)", 乐视的adx的request没有HTTP的头
    case 3:
        nr.setRawHeader(QByteArray("x-openrtb-version"),QByteArray("2.3.1"));   //"科大讯飞广告交易平台(Iflytek)"
        break;
    case 4:
        nr.setRawHeader(QByteArray("x-adviewrtb-version"),QByteArray("2.3"));   //"AdView移动广告优化平台(AdView)"
        break;
    }
}

void Amax_Test::setSubmitMethodSlot()
{
    int index = ui->methodComboBox->currentIndex();
    switch (index) {
    case 0:
        /*setDisabled(ui->requestTextEdit,false);
        setDisabled(ui->autoFormatJsonCheckBox,false);
        setDisabled(ui->autoSaveResponseCheckBox,false);
        setDisabled(ui->readLogAndSaveResponsePushButton,false);
        setDisabled(ui->readRequestPushButton,false);*/
        break;
    default:
        resetSlot();
        /*setChecked(ui->autoFormatJsonCheckBox,false);
        setChecked(ui->autoSaveResponseCheckBox,false);
        setDisabled(ui->requestTextEdit,true);
        setDisabled(ui->autoFormatJsonCheckBox,true);
        setDisabled(ui->autoSaveResponseCheckBox,true);
        setDisabled(ui->readLogAndSaveResponsePushButton,true);
        setDisabled(ui->readRequestPushButton,true);
        */
        break;
    }
}



void Amax_Test::setDataTypeSlot()
{
    int index = ui->platComboBox->currentIndex();
    ui->typeLineEdit->clear();
    if(index >= 0 && index <= 4)
        ui->typeLineEdit->setText("JSON字符串类型");
    else
       ui->typeLineEdit->setText("ProtoBuf二进制类型");
}

void Amax_Test::testByStressSlot()
{
    SendRequestByParallel_Dialog srbp;
    Thread *pthread = NULL;
    if(srbp.exec() == QDialog::Accepted)
    {
        int numThread = srbp.numThread;  //线程数
        int numRequest = srbp.numRequest;  //每个线程发送请求数
        //ui->sendByBatchProgressBar->setRange(0,numThread);
        pthread = new Thread[numThread];
        for(int i = 0 ; i < numThread ; ++i )
        {
            pthread[i].set(numRequest);   //创建线程
            pthread[i].start();        //线程开始
            //ui->sendByBatchProgressBar->setValue(i+1);
        }
        for(int i = 0 ; i < numThread ; ++i )
        {
            pthread[i].wait();         //等待所有线程执行完成
        }
        QString count;
        QString text = "并发测试已完成：\n请求数："+count.setNum(numRequest)+"条"+"\n线程数："+count.setNum(numThread)+"个";
        QMessageBox message(QMessageBox::Information, "请求并发测试完成", text, QMessageBox::Ok, NULL);
        message.exec();
    }
    else
    {
        //点击取消需要处理情况
        goto exit;
    }
exit:
    if(pthread != NULL)
    {
        delete[] pthread;
        pthread = NULL;
    }
}

void Amax_Test::writeResponseSlot()
{
    QFile *outFile = NULL;
    QTextStream *out = NULL;
    if(writeFile(this, "Text files(*.txt)", QDir::currentPath(), QIODevice::WriteOnly, outFile, out))
    {
        (*out)<<ui->responseTextEdit->toPlainText();
    }
    if(out != NULL)
    {
        delete out;
    }
    if(outFile != NULL)
    {
        outFile->close();
        delete outFile;
    }
}
