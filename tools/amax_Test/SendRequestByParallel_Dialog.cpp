#include "SendRequestByParallel_Dialog.h"
#include "ui_SendRequestByParallel_Dialog.h"

SendRequestByParallel_Dialog::SendRequestByParallel_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendRequestByParallel_Dialog)
{
    ui->setupUi(this);

    QObject::connect(ui->cancelPushButton,SIGNAL(clicked()),this,SLOT(cancelSlot()));   //取消
    QObject::connect(ui->assignPushButton,SIGNAL(clicked()),this,SLOT(assignSlot()));   //确定
}

void SendRequestByParallel_Dialog::cancelSlot()
{
    this->close();
}

void SendRequestByParallel_Dialog::assignSlot()
{
    numThread = ui->numThreadSpinBox->value();
    numRequest = ui->numRequestByThreadSpinBox->value();
    this->close();
}

SendRequestByParallel_Dialog::~SendRequestByParallel_Dialog()
{
    delete ui;
}
