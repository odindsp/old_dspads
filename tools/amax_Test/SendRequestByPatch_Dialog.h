#ifndef SENDREQUESTBYPATCH_DIALOG_H
#define SENDREQUESTBYPATCH_DIALOG_H

#include <QDialog>

namespace Ui {
class SendRequestByPatch_Dialog;
}

class SendRequestByPatch_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendRequestByPatch_Dialog(QWidget *parent = 0);
    ~SendRequestByPatch_Dialog();

public:
    Ui::SendRequestByPatch_Dialog *ui;
    unsigned int numRequest;
private slots:
    void cancelSlot();      //取消
    void assignSlot();      //确定
};

#endif // SENDREQUESTBYPATCH_DIALOG_H
