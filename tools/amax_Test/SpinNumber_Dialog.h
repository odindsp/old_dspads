#ifndef SPINNUMBER_H
#define SPINNUMBER_H

#include <QDialog>

namespace Ui {
class SpinNumber_Dialog;
}

class SpinNumber_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpinNumber_Dialog(QWidget *parent = 0);
    ~SpinNumber_Dialog();

public:
    Ui::SpinNumber_Dialog *ui;
    unsigned int g_num;

private slots:
    void cancelSlot();      //取消
    void assignSlot();      //确定
};

#endif // SPINNUMBER_H
