#include "mainwindow.h"

#include <QApplication>

#include <stdio.h>
#include <Windows.h>
#include <QMessageBox>
#include <QTextStream>
#include <QSettings>
#include <QDateTime>
#include <QFile>
#include "type.h"


QString log_file = "";

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)

{

    QString text;

    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;

    case QtWarningMsg:
        text = QString("Warning:");
        break;

    case QtCriticalMsg:
        text = QString("Critical:");
        break;

    case QtFatalMsg:
        text = QString("Fatal:");

    }

    QDateTime current_date_time = QDateTime::currentDateTime();

    QString current_date = current_date_time.toString("yyyy-MM-dd hh:mm:ss ddd");

    QString message = text.append(msg).append("(").append(current_date).append(")");

    QFile::remove(log_file);
    QFile file(log_file);

    file.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream text_stream(&file);

    text_stream<<message<<"\r\n";

    file.close();

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    WSADATA  Ws;
    SOCKET ClientSocket;
    struct sockaddr_in ClientAddr;
    int Ret = 0;
    int timeout = 10000;
    bool isAlive = true;

    QSettings *setting = new QSettings("client.ini",QSettings::IniFormat);

    QString str_ip = setting->value("/server/server_ip").toString();
    log_file = setting->value("/log/log_path").toString();

    qInstallMessageHandler(outputMessage);

    char *ip = NULL;
    QByteArray ba = str_ip.toLatin1();
    ip=ba.data();
    int port = setting->value("/server/server_port").toInt();
    // Init Windows Socket
    if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
    {
        printf("Init Windows Socket Failed::%ld\n", GetLastError());
        //return -1;
    }
    // Create Socket
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET)
    {
        printf("Create Socket Failed::%ld\n", GetLastError());
        //return -1;
    }
    ClientAddr.sin_family = AF_INET;
    #ifdef DEBUG
    ClientAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    ClientAddr.sin_port = htons(PORT);
    #endif
    ClientAddr.sin_addr.s_addr = inet_addr(ip);
    ClientAddr.sin_port = htons(port);
    memset(ClientAddr.sin_zero, 0x00, 8);

    setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));

    // connect socket
    Ret = connect(ClientSocket, (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
    if (Ret == SOCKET_ERROR)
    {

        printf("Connect Error::%ld\n", GetLastError());
        qDebug("Connect server Error!");
        //return -1;
    }
    else
    {
        qDebug("Connect server succedded!");
    }

    MainWindow w;
    w.setWindowTitle("数据对比工具");
    w.init(ClientSocket,Ret,isAlive);
    w.show();

    return a.exec();
}
