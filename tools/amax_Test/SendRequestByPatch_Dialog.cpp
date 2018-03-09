#include "SendRequestByPatch_Dialog.h"
#include "ui_SendRequestByPatch_Dialog.h"

SendRequestByPatch_Dialog::SendRequestByPatch_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendRequestByPatch_Dialog)
{
    ui->setupUi(this);

    QObject::connect(ui->cancelPushButton,SIGNAL(clicked()),this,SLOT(cancelSlot()));   //取消
    QObject::connect(ui->assignPushButton,SIGNAL(clicked()),this,SLOT(assignSlot()));   //确定
}

void SendRequestByPatch_Dialog::cancelSlot()
{
    this->close();
}

void SendRequestByPatch_Dialog::assignSlot()
{
    numRequest = ui->numRequestByPatchSpinBox->value();
    this->close();
}

SendRequestByPatch_Dialog::~SendRequestByPatch_Dialog()
{
    delete ui;
}
