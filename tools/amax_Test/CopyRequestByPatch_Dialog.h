#ifndef COPYREQUESTBYPATCH_DIALOG_H
#define COPYREQUESTBYPATCH_DIALOG_H

#include <QDialog>

namespace Ui {
class CopyRequestByPatch_Dialog;
}

class CopyRequestByPatch_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit CopyRequestByPatch_Dialog(QWidget *parent = 0);
    ~CopyRequestByPatch_Dialog();

public:
    Ui::CopyRequestByPatch_Dialog *ui;
    unsigned int  numRequest;
private slots:
    void cancelSlot();      //取消
    void assignSlot();      //确定
};

#endif // COPYREQUESTBYPATCH_DIALOG_H
