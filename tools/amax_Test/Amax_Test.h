#ifndef AMAX_TEST_H
#define AMAX_TEST_H

#include "Thread.h"
#include "util.h"

#include <QFile>
#include <QLineEdit>
#include <QNetworkReply>
#include <QStyledItemDelegate>
#include <QString>
#include <QTextEdit>
#include <QTextStream>
#include <QWidget>
#include <QVector>

namespace Ui {
class Amax_Test;
}

class Amax_Test : public QWidget
{
    Q_OBJECT

public:
    explicit Amax_Test(QWidget *parent = 0);
    //bool do_get(QUrl url,QMap<QString,QString> &m);     //get提交方法
    //bool do_post(QUrl url,QString json,bool isbatch,QMap<QString,QString> &m);       //post提交方法
    bool eventFilter(QObject *, QEvent *);  //事件过滤器
   // QString formatText(QString json);       //格式化Json
   // QMap<QString,QString> finishReply(QNetworkReply *reply);        //返回Response
    QPalette getPalette(QColor color);
    QString getText(QWidget *widget);       //获取文本
    QUrl getUrl();  //获取Url
    bool isUrl(QUrl& url);      //校验服务器的Url
    //bool isJson(QString& json);     //校验Json
    bool isChecked(QWidget *widget);
    void initThread();      //初始化线程
    void insertTip(QWidget *widget,QString info);       //插入提示语
    bool openOrSaveFile(QString caption,const char* filter,QIODevice::OpenMode mode);       //打开或者保存文件
    QMap<QString,QString> parseStatusCode(QNetworkReply *reply);        //解析http返回码
    void removeTip(QWidget *widget, QString info);          //取消提示语
    bool setChecked(QWidget* widget,bool checked);      //设置勾选
    bool setDisabled(QWidget *widget, bool disabled);
    void setPalette(QWidget *widget,QPalette palette);      //设置背景色
    void setPlatformHeader(QNetworkRequest& nr);    //设置json的http头
    void setText(QWidget *widget,QString string);       //设置文本
    ~Amax_Test();
public:
    Ui::Amax_Test *ui;
    QStyledItemDelegate* itemDelegate;
private slots:
    void readRequestSlot();         //读取Request文件
    void sendRequestByBatchSlot();       //读取Log日志文件
    void resetSlot();   //清空TextEdit
    void sendRequestSlot();         //发送Request
    void setDataTypeSlot();      //设置相应的数据类型
    void setSubmitMethodSlot();     //设置提交方法:POST或者GET
    void testByStressSlot();            //压力测试
    void writeResponseSlot();       //保存Response文件
    void copyDataSlot();            //复制数据
};

#endif // AMAX_TEST_H
