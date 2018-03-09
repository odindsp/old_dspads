#include "SendByPatch_Dialog.h"
#include "ui_SendByPatch_Dialog.h"

SpinNumber_Dialog::SpinNumber_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpinNumber_Dialog)
{
    ui->setupUi(this);

    QObject::connect(ui->cancelPushButton,SIGNAL(clicked()),this,SLOT(cancelSlot()));   //取消
    QObject::connect(ui->assignPushButton,SIGNAL(clicked()),this,SLOT(assignSlot()));   //确定
}


void SpinNumber_Dialog::cancelSlot()
{
    this->close();
}

void SpinNumber_Dialog::assignSlot()
{
    g_numThread = ui->numThreadSpinBox->value();
    this->close();
}

SpinNumber_Dialog::~SpinNumber_Dialog()
{
    delete ui;
}
