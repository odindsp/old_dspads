#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QValidator>
#include <QProgressBar>
#include <QDebug>

#include <QFile>
#include <QTextCodec>
#include "type.h"


#include "ui_mainwindow.h"

#include <stdio.h>
#include <Windows.h>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>

#include <QAxObject>
#include <QScopedPointer>
#include <QTableWidgetItem>
#include <QInputDialog>

extern QString log_file;
MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    m_url = new QUrl();
    m_url->setScheme("ftp");
    m_url->setPort(21);
    m_url->setUserName(FTP_USER);
    m_url->setPassword(FTP_PASSWD);
    m_manager = new QNetworkAccessManager(this);

    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    // close socket
    if(m_manager != NULL)
    {
        delete m_manager;
        m_manager = NULL;
    }
    if(m_url != NULL)
    {
        delete m_url;
        m_url = NULL;
    }
    isAlive = false;
    if(!v_email.empty())
    {
        v_email.clear();
        v_tmpemail.clear();
    }
    if(!v_group.empty())
    {
        v_group.clear();
        v_tmpgroup.clear();
    }
    closesocket(m_client);
    qDebug("close connect to server!");
    QFile::remove(log_file);
    WSACleanup();
    delete ui;
}

void MainWindow::init(SOCKET client,int status,bool flag)
{
    m_client = client;
    m_sta_conect = status;
    isAlive = flag;
}

void MainWindow::upload(const QString &file, const QString &filename)
{
    QTextCodec *codec= QTextCodec::codecForName("UTF-8");

    QFile txt1(file);
    txt1.open(QIODevice::ReadOnly);

    //qDebug()<<txt1.isOpen();
    QByteArray by_txt=txt1.readAll();
    txt1.close();
    //qDebug()<<by_txt;

    QString name =  QString::fromLatin1(codec->fromUnicode(filename));

    QString upfile = FTP_PATH + name;
    QUrl u(upfile);
    u.setPort(21);
    u.setUserName(FTP_USER);
    u.setPassword(FTP_PASSWD);

    QString path = u.path();
    QString upPath = FILE_PATH + name;
    m_url->setPath(upPath);


    if(m_manager != NULL)
    {
        m_manager->put(QNetworkRequest(u), by_txt);
        //m_manager->put(QNetworkRequest(*m_url), by_txt);
    }

}

void MainWindow::download(const QString &path,const QString &filename)
{
    QDir dir(path);
    QString str;

    if(!dir.exists())
    {
        str = dir.currentPath();
        str += path;
        dir.mkdir(str);
    }
    str = dir.absolutePath();


    QFileInfo info;
    info.setFile(dir, filename);
    QString newfile = info.filePath();

    QString downfile = FTP_PATH + filename;

    m_pFile = new QFile(newfile);
    m_pFile->open(QIODevice::Append|QIODevice::WriteOnly);
    //m_url->setPath(downfile);

    QUrl u(downfile);
    u.setPort(21);
    u.setUserName(FTP_USER);
    u.setPassword(FTP_PASSWD);
    connect(m_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(finished(QNetworkReply *)));

    if(m_manager != NULL)
    {
        m_pReply = m_manager->get(QNetworkRequest(u));
    }
}

void MainWindow::on_pushButton_2_clicked()
{

    QTextCodec *codec= QTextCodec::codecForName("UTF-8");
    int nRet = 0;
    QString groupIndex;
    QString groupInfo = ui->comboBox->currentText();
    if(groupInfo == "项目")
    {
        groupIndex = "1";
    }
    else if(groupInfo == "推广组")
    {
        groupIndex = "2";
    }
    else if(groupInfo == "创意")
    {
        groupIndex = "3";
    }
    QString groupid = ui->lineEdit->text();
    QString date = ui->lineEdit_2->text();
    QString type = ui->comboBox_2->currentText();
    QString email = ui->lineEdit_3->text();

    if(m_sta_conect != 0)
    {
        QMessageBox::critical(NULL, "Error", ERROR_SERVER_CONNECT_FAILED, QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if(groupid == NULL || date == NULL || email == NULL)
    {
        if(groupid == NULL)
        {
            QMessageBox::warning(NULL, "warning", ERROR_CREATIVE_EMPTY_STRING, QMessageBox::Yes, QMessageBox::Yes);
        }
        else if(date == NULL)
        {
            QMessageBox::warning(NULL, "warning", ERROR_TIME_EMPTY_STRING, QMessageBox::Yes, QMessageBox::Yes);
        }

        else if(email == NULL)
        {
            QMessageBox::warning(NULL, "warning", ERROR_EMAIL_EMPTY_STRING, QMessageBox::Yes, QMessageBox::Yes);

        }
        return;
    }

    nRet = filterData(groupid,date,m_email);
    if(nRet != 0)
    {
        return;
    }
    bool f_ret = processFile();
    QString name;
    if(!f_ret)
    {
        QMessageBox::about(NULL, "Info", "处理数据文件失败");
        qDebug("处理数据文件失败!");
        return;
    }
    else
    {
        qDebug("处理并上传数据文件成功!");
        name =  QString::fromLatin1(codec->fromUnicode(m_filename));
    }

    //QMessageBox::warning(NULL, "warning", m_filename, QMessageBox::Yes, QMessageBox::Yes);

    QString admasterId;
    QString adInfo = ui->comboBox_3->currentText();
    if(adInfo == "admaster")
    {
        admasterId = "1";
    }
    else if(adInfo == "秒针")
    {
        admasterId = "2";
    }

    QString target;
    QString tarInfo = ui->comboBox_4->currentText();
    if(tarInfo == "数据")
        {
        target = "1";
    }
    else if(tarInfo == "地域")
        {
        target = "2";
    }

    if(name == NULL)
    {
        QMessageBox::warning(NULL, "warning", ERROR_UPFILE_EMPTY_STRING, QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    else
    {

        QFileInfo fileinfo;
        fileinfo = QFileInfo(m_filename);
        //文件后缀
        QString file_suffix = fileinfo.suffix();

        if(file_suffix == NULL)
        {
           QMessageBox::warning(NULL, "warning", ERROR_UPFILE_FORMAT_STRING, QMessageBox::Yes, QMessageBox::Yes);
           return;
        }
        if(m_index == NULL)
        {
            bool isOK;
            m_index = QInputDialog::getText(NULL, "Input Dialog","请输入排查需要的关键列信息(逗号分隔)",
                                                        QLineEdit::Normal,"your comment",&isOK);
//            if(isOK)
//            {
//                   QMessageBox::information(NULL, "Information", "Your comment is: <b>" + m_index + "</b>",
//                                                       QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes);
//            }
        }
    }
    QString sendStr = date + " " + type + " " + groupIndex + " " + groupid + " " + admasterId  + " "  + name + " " + m_index + " " + target + " " + m_email;

    if(nRet == 0)
    {

        char *SendBuffer = NULL;

        QByteArray ba = sendStr.toLatin1();

        SendBuffer=ba.data();

        // send data to server
        int Ret = 0;
        char buffer[BUFFER_SIZE];


        Ret = send(m_client, SendBuffer, (int)strlen(SendBuffer), 0);
        //QMessageBox::about(NULL, "Info", QString::number(Ret));

        QMessageBox *box=new QMessageBox(this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        QTimer::singleShot(100, box, SLOT(close()));

        if (Ret == SOCKET_ERROR)
        {
            printf("Send Info Error::%ld\n", GetLastError());
            box->information(this,"Info","data send failed, maybe server is closed!");
        }
        else if(Ret != SOCKET_ERROR && ERROR_IO_PENDING != WSAGetLastError())
        {
            box->information(this,"Info",OK_STRING);
            qDebug("send data to server successed!");
            qDebug(SendBuffer);
        }

    }
    if(m_fileInfo != "")
    {
        m_fileInfo = "";
    }
    if(m_filename != "")
    {
        m_filename = "";
    }
    if(m_index != "")
    {
        m_index = "";
    }

}

void MainWindow::on_pushButton_clicked()
{

    QString file_full = QFileDialog::getOpenFileName(this,
                            tr("Open Spreadsheet"), ".",
                            tr("Spreadsheet files (*.*)"));
     m_fileInfo = file_full;
     m_index = "";

}

void MainWindow::on_lineEdit_editingFinished()
{
    QString groups = ui->lineEdit->text();
    QString tmpgroup;
    if(groups != NULL)
    {
        QString sub = "，";
        if(groups.contains(sub))
        {
            while(groups.contains(sub))
            {
                int pos = groups.indexOf(sub);

                tmpgroup = groups.mid(0,pos);
                groups = groups.mid(pos+1);
                if(tmpgroup != NULL)
                {
                    tmpgroup += ",";
                }
            }
            tmpgroup += groups;
        }else
        {
            tmpgroup = groups;
        }

        m_groups = tmpgroup;

        if(!v_tmpgroup.empty())
        {
            v_tmpgroup.clear();
        }
        if(tmpgroup.indexOf(","))
        {
            int k = tmpgroup.indexOf(",");
            while(k != -1)
            {

                int sub_pos = tmpgroup.indexOf(",");
                QString sub_group = tmpgroup.mid(0,sub_pos);
                v_group.push_back(sub_group);
                v_tmpgroup.push_back(sub_group);
                tmpgroup = tmpgroup.mid(sub_pos+1);
                k = tmpgroup.indexOf(",");

            }
        }
        v_group.push_back(tmpgroup);
        v_tmpgroup.push_back(tmpgroup);


//        int n = v_group.size();
//        QString strn = QString::number(n);
//        QMessageBox::about(NULL, "Info", strn);

        QVector<QString>::iterator iter = v_group.begin();
        for(int i = 0; i < v_group.size(); ++i)
        {
            QString group = v_group.at(i);
//            QMessageBox::about(NULL, "Info", group);

            QRegExp regExp("[0-9a-zA-Z]{8}\(-[0-9a-fA-Z]{4}\){4}[0-9a-zA-Z]{8}");

            QRegExpValidator validator(regExp,this);
            int pos = 0;
            if(QValidator::Acceptable != validator.validate(group,pos))
            {
                ui->lineEdit->setText(ERROR_CREATIVE_FORMAT_STRING);
            }
        }
        v_group.clear();
    }

}

void MainWindow::on_lineEdit_2_editingFinished()
{

    int pos = 0;
    QString date = ui->lineEdit_2->text();
    if(date != NULL)
    {
        if(date.size() != 8)
        {
            ui->lineEdit_2->setText(ERROR_TIME_FORMAT_STRING);
        }
        else
        {
            QRegExp regExp("^2[0-9]{3}(0?[1-9]|1[0-2])((0?[1-9])|((1|2)[0-9])|30|31)$");

            QRegExpValidator validator(regExp,this);
            if(QValidator::Acceptable != validator.validate(date,pos))
            {
               ui->lineEdit_2->setText(ERROR_TIME_FORMAT_STRING);

            }
        }
    }
}

void MainWindow::finished(QNetworkReply * reply)
{
    m_pFile->write(reply->readAll());
    m_pFile->flush();
    m_pFile->close();
    reply->deleteLater();
}

bool MainWindow::ExcelToCsvFile(const QString &excelFileName, const QString &csvFileName)
{
//    QMessageBox::about(NULL, "Info", excelFileName);
//    QMessageBox::about(NULL, "Info", csvFileName);

    if (!QFile::exists(excelFileName))
        return false;

    // 当pApplication析构的时候会将其所有相关的子对象都清理
    QScopedPointer<QAxObject> pApplication(new QAxObject());

    // 设置连接Excel控件，需要在GUI的环境下才能成功
    bool ok = pApplication->setControl("Excel.Application");
    if (!ok)
        return false;

    pApplication->dynamicCall("SetVisible(bool)", false); // false表示不显示窗体
    pApplication->setProperty("DisplayAlerts", false); // false表示不显示(输出)任何警告信息。
    QAxObject *pWorkBooks = pApplication->querySubObject("Workbooks"); // Excel工作薄(对象)
    if (pWorkBooks == 0)
        return false;

    QAxObject *pWorkBook  = pWorkBooks->querySubObject("Open(const QString &)", excelFileName); // 打开一个Excel文件
    if (pWorkBook == 0)
        return false;

    QAxObject *pSheets = pWorkBook->querySubObject("WorkSheets"); // Excel工作表集
    if (pSheets == 0)
        return false;

    QAxObject *pSheet = pSheets->querySubObject("Item(int)", 1); // 得到指定索引的工作表
    if (pSheet == 0)
        return false;


    int sheet_count = pSheets->property("Count").toInt();  //获取工作表数目
    //qDebug()<<sheet_count<<endl;


    QAxObject *last_sheet = pSheets->querySubObject("Item(int)", sheet_count);
    QAxObject *work_sheet = pSheets->querySubObject("Add(QVariant)", last_sheet->asVariant());
    last_sheet->dynamicCall("Move(QVariant)", work_sheet->asVariant());

    QAxObject *worksheet = pWorkBook->querySubObject("WorkSheets(int)",1);//获取第一个工作表
    //QAxObject *used_range = last_sheet->querySubObject("UsedRange");     //获得利用的范围

    QAxObject *used_range = worksheet->querySubObject("UsedRange");     //获得利用的范围
    QAxObject *rows  = used_range->querySubObject("Rows");
    QAxObject *columns = used_range->querySubObject("Columns");

    int row_start = used_range->property("Row").toInt();
    //qDebug()<<row_start;


    int column_start  = used_range->property("Column").toInt();     //获得开始列
    int row_count = rows->property("Count").toInt();
    //qDebug()<<row_count;              //已经验证准确
    int column_count = columns->property("Count").toInt();
    char buf[128] = "";
    m_index = "";

    for(int j = column_start;j<=column_count;j++)
    {
        QAxObject *cell = worksheet->querySubObject("Cells(int,int)",1,j);

        QString value = cell->dynamicCall("Value2()").toString();
        if(value == "自定义信息")
        {
            if(m_index != "")
            {
                sprintf(buf,",%d",j);
            }else
            {
                sprintf(buf,"%d",j);
            }
        }
        else if(value.toLower() == "ip")
        {
            if(m_index != "")
            {
                sprintf(buf,",%d",j);
            }else
            {
                sprintf(buf,"%d",j);
            }
        }
        if(buf != "")
        {
            m_index += buf;
            memset(buf,0,128);
        }

        cell->dynamicCall("delete");

    }

    // 另存为文件, 3:txt文件（空格分隔）| 6:csv文件（逗号分隔）
    pSheet->dynamicCall("SaveAs(const QString&, int)", QDir::toNativeSeparators(csvFileName), 6);

    pApplication->dynamicCall("Quit()");

    return true;
}

bool MainWindow::processFile()
{
    QString file_full = m_fileInfo;
    if (!file_full.isEmpty())
    {
        QString file_name,file_path,file_suffix,tmpfile,csvname;
        QFileInfo fileinfo;
        fileinfo = QFileInfo(file_full);
        //文件名
        file_name = fileinfo.fileName();
        file_path = fileinfo.absolutePath();
        file_suffix = fileinfo.suffix();

        if(file_path.size() < 4)
        {
            m_file = file_path + file_name;
        }else
        {
            m_file = file_path + "/" + file_name;
        }

        int pos = file_name.indexOf(file_suffix);



//        char *filebuf = NULL;
//        QByteArray ba = file_name.toLatin1();
//        filebuf=ba.data();
//        char buf[512] = "";
//        sprintf(buf,"data file %s is handle and upload ,please wait",filebuf);

//        QString msg = QString(QLatin1String(buf));
//        qDebug(buf);
//        QMessageBox::about(NULL, "Info", "data file is handle and upload ,please wait");


        if(file_suffix == "xlsx" || file_suffix == "xls")
        {
            tmpfile = file_name.mid(0,pos);
            csvname = tmpfile + "csv";
        }
        else if(file_suffix == "csv")
        {
            tmpfile = file_name.mid(0,pos-1);
            csvname = tmpfile + "_bak." + "csv";
        }
        QString csv_file;
        if(file_path.size() < 4)
        {
            csv_file = file_path + csvname;
        }else
        {
            csv_file = file_path + "/" + csvname;
        }
        QMessageBox::about(NULL, "Info", "正在转换并上传文件，请稍等");
        if(ExcelToCsvFile(m_file, csv_file))
        {
//            file_name = csvname;
//            m_file = file_path + "/" +file_name;
            m_file = csv_file;
            m_filename = csvname;
            upload(m_file,csvname);
            QFile::remove(m_file);
            return true;
        }
        else
        {
            return false;
        }
    }
}


void MainWindow::on_lineEdit_3_editingFinished()
{
    QString email = ui->lineEdit_3->text();
    QString tmpemail;
    if(email != NULL)
    {
        QString sub = "，";
        QString s_email;
        if(email.contains(sub))
        {
            while(email.contains(sub))
            {
                int pos = email.indexOf(sub);

                tmpemail = email.mid(0,pos);
                email = email.mid(pos+1);
                if(tmpemail != NULL)
                {
                    tmpemail += ",";
                }
            }
            tmpemail += email;
        }else
        {
            tmpemail = email;
        }

        m_email = tmpemail;

        if(!v_tmpemail.empty())
        {
            v_tmpemail.clear();
        }
        if(tmpemail.indexOf(","))
        {
            int k = tmpemail.indexOf(",");
            while(k != -1)
            {

                int sub_pos = tmpemail.indexOf(",");
                QString sub_email = tmpemail.mid(0,sub_pos);
                v_email.push_back(sub_email);
                v_tmpemail.push_back(sub_email);
                tmpemail = tmpemail.mid(sub_pos+1);
                k = tmpemail.indexOf(",");

            }
        }
        v_email.push_back(tmpemail);
        v_tmpemail.push_back(tmpemail);


/*        int n = v_email.size();
        QString strn = QString::number(n);
        QMessageBox::about(NULL, "Info", strn)*/;

        QVector<QString>::iterator iter = v_email.begin();
        for(int i = 0; i < v_email.size(); ++i)
        {
            QString s_email = v_email.at(i);

            QRegExp regExp("^[a-zA-Z0-9_-]+@[a-zA-Z0-9_-]+(\.[a-zA-Z0-9_-]+)+$");

            QRegExpValidator validator(regExp,this);
            int epos = 0;
            if(QValidator::Acceptable != validator.validate(s_email,epos))
            {
                ui->lineEdit_3->setText(ERROR_EMAIL_FORMAT_STRING);
            }
        }
        v_email.clear();
    }

}

int MainWindow::filterData(const QString &groupid, const QString &date, const QString &email)
{
    int nRet = 0;


    int pos = 0;
    QString m_groupid = groupid;
    QString m_date = date;
    if(groupid != NULL)
    {
//        int n = v_tmpgroup.size();
//        QString strn = QString::number(n);
//        QMessageBox::about(NULL, "Info", strn);

        QVector<QString>::iterator iter = v_tmpgroup.begin();
        for(int i = 0; i < v_tmpgroup.size(); ++i)
        {
            QString s_group = v_tmpgroup.at(i);
//            QMessageBox::warning(NULL, "warning",s_group, QMessageBox::Yes, QMessageBox::Yes);
            QRegExp regExp("[0-9a-zA-Z]{8}\(-[0-9a-fA-Z]{4}\){4}[0-9a-zA-Z]{8}");

            QRegExpValidator validator(regExp,this);
            int epos = 0;
            if(QValidator::Acceptable != validator.validate(s_group,epos))
            {
                QMessageBox::warning(NULL, "warning", ERROR_CREATIVE_FORMAT_STRING, QMessageBox::Yes, QMessageBox::Yes);
                nRet = -1;
            }
        }
        v_tmpgroup.clear();
    }

    if(date != NULL && nRet == 0)
    {
        if(date.size() != 8)
        {
            QMessageBox::warning(NULL, "warning", ERROR_TIME_FORMAT_STRING, QMessageBox::Yes, QMessageBox::Yes);
            nRet = -1;
        }
        else
        {
            QRegExp regExp("^2[0-9]{3}(0?[1-9]|1[0-2])((0?[1-9])|((1|2)[0-9])|30|31)$");

            QRegExpValidator validator(regExp,this);
            if(QValidator::Acceptable != validator.validate(m_date,pos))
            {
               QMessageBox::warning(NULL, "warning", ERROR_TIME_FORMAT_STRING, QMessageBox::Yes, QMessageBox::Yes);
               nRet = -1;
            }
        }
    }

    if(email != NULL)
    {

        QVector<QString>::iterator iter = v_tmpemail.begin();
        for(int i = 0; i < v_tmpemail.size(); ++i)
        {
            QString s_email = v_tmpemail.at(i);
            //QMessageBox::warning(NULL, "warning",s_email, QMessageBox::Yes, QMessageBox::Yes);

            QRegExp regExp("^[a-zA-Z0-9_-]+@[a-zA-Z0-9_-]+(\.[a-zA-Z0-9_-]+)+$");

            QRegExpValidator validator(regExp,this);
            int epos = 0;
            if(QValidator::Acceptable != validator.validate(s_email,epos))
            {
                ui->lineEdit_3->setText(ERROR_EMAIL_FORMAT_STRING);
                nRet = -1;
            }
        }
        v_tmpemail.clear();
    }

    return nRet;
}
