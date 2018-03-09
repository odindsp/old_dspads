#include "CopyRequestByPatch_Dialog.h"
#include "ui_CopyRequestByPatch_Dialog.h"

CopyRequestByPatch_Dialog::CopyRequestByPatch_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CopyRequestByPatch_Dialog)
{
    ui->setupUi(this);
    QObject::connect(ui->cancelPushButton,SIGNAL(clicked()),this,SLOT(cancelSlot()));   //取消
    QObject::connect(ui->assignPushButton,SIGNAL(clicked()),this,SLOT(assignSlot()));   //确定
}

void CopyRequestByPatch_Dialog::cancelSlot()
{
    this->close();
}

void CopyRequestByPatch_Dialog::assignSlot()
{
    numRequest = ui->copyRequestSpinBox->value();
    this->close();
}

CopyRequestByPatch_Dialog::~CopyRequestByPatch_Dialog()
{
    delete ui;
}
