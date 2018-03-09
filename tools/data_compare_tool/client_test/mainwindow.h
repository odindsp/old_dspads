#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QThread>
#include <Windows.h>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressBar>
#include <QMessageBox>


namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void init(SOCKET client,int status,bool falg);
    int filterData(const QString &groupid, const QString &date, const QString &email);
    void upload(const QString &upfile,const QString &file);
    void download(const QString &downPath,const QString &file);
    bool processFile();
    bool ExcelToCsvFile(const QString &excelFileName, const QString &csvFileName);

    ~MainWindow();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void finished(QNetworkReply * reply);

    void on_lineEdit_3_editingFinished();

private:
    Ui::MainWindow *ui;
    SOCKET m_client;
    int m_sta_conect;
    QString m_file;
    QString m_fileInfo;
    QString m_groups;
    QString m_filename;
    QString m_index;
    QString m_email;
    QVector<QString> v_email;
    QVector<QString> v_tmpemail;

    QVector<QString> v_group;
    QVector<QString> v_tmpgroup;


    QNetworkAccessManager *m_manager;
    QNetworkReply *m_pReply;
    QFile *m_pFile;
    QUrl *m_url;

    bool isAlive;
};

#endif // MAINWINDOW_H
