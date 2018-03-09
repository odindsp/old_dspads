#ifndef SENDREQUESTBYPARALLEL_DIALOG_H
#define SENDREQUESTBYPARALLEL_DIALOG_H

#include <QDialog>

namespace Ui {
class SendRequestByParallel_Dialog;
}

class SendRequestByParallel_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendRequestByParallel_Dialog(QWidget *parent = 0);
    ~SendRequestByParallel_Dialog();

public:
    Ui::SendRequestByParallel_Dialog *ui;
    unsigned int numThread;         //线程数
    unsigned int numRequest;        //每个线程发送的请求数

private slots:
    void cancelSlot();      //取消
    void assignSlot();      //确定
};

#endif // SENDREQUESTBYPARALLEL_DIALOG_H
