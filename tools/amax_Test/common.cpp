#include "common.h"
#include <QMutex>

bool writeFile(QWidget *widget, const char *filter, QString dir, QIODevice::OpenMode mode, QFile *&outFile, QTextStream *&out)
{
    QString fileName;
    bool bRet;
    fileName = QFileDialog::getSaveFileName(widget,QString("请选择需要写入的文本文件"),dir,QString(filter));
    if(fileName.isNull() || fileName.isEmpty())
        return false;
    outFile = new QFile;
    outFile->setFileName(fileName);
    bRet = outFile->open(mode);
    if( ! bRet)
        return false;
    out = new QTextStream(outFile);
    return true;
}

bool readFile(QWidget *widget, const char *filter, QString dir, QIODevice::OpenMode mode, QFile *&inFile, QTextStream *&in)
{
    QString fileName;
    bool bRet;
    fileName = QFileDialog::getOpenFileName(widget,"请选择需要读取的文本文件",dir,QString(filter));
    if(fileName.isNull() || fileName.isEmpty())
        return false;
    inFile = new QFile;
    inFile->setFileName(fileName);
    bRet = inFile->open(mode);
    if( ! bRet)
        return false;
    in = new QTextStream(inFile);
    return true;
}

QString formatText(QString json)
{
    json = json.simplified();   //去掉\r、\t、\n等转义字符
    json = json.trimmed();      //去掉空格
    QString formatJson(""); //格式化后字符串
    QString element;
    //i是json当前的字符计数，j是完全匹配计数，k是空格计数
    for(int i = 0, j = 0, k = 0, ii ; i < json.length(); ++i)
    {
        element = QString(json.at(i));    //获取每个字符
        if(j%2 ==0 && element == "}")
        {
            --k;
            for(ii = 0; ii < k; ++ii)
                element += "    ";
            element += "\n";
        }
        else if(j%2 == 0 && element == "{")
        {
            element += "\n";
            ++k;
            for(ii = 0; ii < k; ++ii)
                element += "    ";
        }
        else if(j%2 == 0 && element == ",")
        {
            element += "\n";
            for(ii = 0; ii < k; ++ii)
                element += "    ";
        }
        else if(element == "\"")
            ++j;
        formatJson += element;
    }
    return formatJson;
}

bool isJson(QString& json)
{
    if(json.trimmed() == "")   //jsonDoc为空
        return false;
    QJsonParseError jsonErr;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json.toLatin1(),&jsonErr);  //转化成QJsonDocument,jsonErr存放错误码
    if(jsonErr.error != QJsonParseError::NoError || ! jsonDoc.isObject())
        return false;
    return true;
}

QMap<QString,QString> parseStatusCode(QNetworkReply *reply)
{
    QMap<QString,QString> m;
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();  //获取返回码
    QString result;
    switch(statusCode)
    {
    case 200:
       result  = "OK（成功）  服务器已成功处理了请求。 通常，这表示服务器提供了请求的网页。";
       break;
    case 404 :
       result = "404   Not Found（未找到） \n服务器找不到请求的网页。";
       break;
    case 503:
       result  = "503   Service Unavailable（服务不可用） \n服务器目前无法使用（由于超载或停机维护）。 通常，这只是暂时状态。";
       break;
    case 502:
       result  = "502   Bad Gateway（错误网关） \n服务器作为网关或代理，从上游服务器收到无效响应。";
       break;
    case 100:
       result  = "100   Continue（继续） \n请求者应当继续提出请求。 服务器返回此代码表示已收到请求的第一部分，正在等待其余部分。";
       break;
    case 101:
       result  = "101   Switching Protocols（切换协议） \n请求者已要求服务器切换协议，服务器已确认并准备切换。";
       break;
    case 201:
       result  = "201   Created（已创建）  \n请求成功并且服务器创建了新的资源。";
       break;
    case 202:
       result  = "202   Accepted（已接受）  \n服务器已接受请求，但尚未处理。";
       break;
    case 203:
       result  = "203   Non-Authoritative Information（非授权信息）  \n服务器已成功处理了请求，但返回的信息可能来自另一来源。";
       break;
    case 204:
       result  = "204   No Content（无内容）  \n服务器成功处理了请求，但没有返回任何内容。";
       break;
    case 205:
       result  = "205   Reset Content（重置内容） \n服务器成功处理了请求，但没有返回任何内容。";
       break;
    case 206:
       result  = "206   Partial Content（部分内容）  \n服务器成功处理了部分 GET 请求。";
       break;
    case 300:
       result  = "300   Multiple Choices（多种选择）  \n针对请求，服务器可执行多种操作。 服务器可根据请求者 (user agent) 选择一项操作，或提供操作列表供请求者选择。";
       break;
    case 301:
       result  = "301   Moved Permanently（永久移动）  \n请求的网页已永久移动到新位置。 服务器返回此响应（对 GET 或 HEAD 请求的响应）时，会自动将请求者转到新位置。";
       break;
    case 302:
       result  = "302   Found（临时移动）  \n服务器目前从不同位置的网页响应请求，但请求者应继续使用原有位置来进行以后的请求。";
       break;
    case 303:
       result  = "303   See Other（查看其他位置） \n请求者应当对不同的位置使用单独的 GET 请求来检索响应时，服务器返回此代码。";
       break;
    case 304:
       result  = "304   Not Modified（未修改） \n自从上次请求后，请求的网页未修改过。 服务器返回此响应时，不会返回网页内容。";
       break;
    case 305:
       result  = "305   Use Proxy（使用代理） \n请求者只能使用代理访问请求的网页。 如果服务器返回此响应，还表示请求者应使用代理。";
       break;
    case 307:
       result  = "307   Temporary Redirect（临时重定向）  \n服务器目前从不同位置的网页响应请求，但请求者应继续使用原有位置来进行以后的请求。";
       break;
    case 400:
       result  = "400   Bad Request（错误请求） \n服务器不理解请求的语法。";
       break;
    case 401:
       result  = "401   Unauthorized（未授权） \n请求要求身份验证。 对于需要登录的网页，服务器可能返回此响应。";
       break;
    case 403:
       result  = "403   Forbidden（禁止） \n服务器拒绝请求。";
       break;
    case 405:
       result  = "405   Method Not Allowed（方法禁用） \n禁用请求中指定的方法。";
       break;
    case 406:
       result  = "406   Not Acceptable（不接受） \n无法使用请求的内容特性响应请求的网页。";
       break;
    case 408:
       result  = "408   Request Timeout（请求超时）  \n服务器等候请求时发生超时。";
       break;
    case 409:
       result  = "409   Conflict（冲突）  \n服务器在完成请求时发生冲突。 服务器必须在响应中包含有关冲突的信息。";
       break;
    case 410:
       result  = "410   Gone（已删除）  \n如果请求的资源已永久删除，服务器就会返回此响应。";
       break;
    case 411:
       result  = "411   Length Required（需要有效长度） \n服务器不接受不含有效内容长度标头字段的请求。";
       break;
    case 412:
       result  = "412   Precondition Failed（未满足前提条件） \n服务器未满足请求者在请求中设置的其中一个前提条件。";
       break;
    case 413:
       result  = "413   Request Entity Too Large（请求实体过大） \n服务器无法处理请求，因为请求实体过大，超出服务器的处理能力。";
       break;
    case 414:
       result  = "414   Request-URI Too Long（请求的 URI 过长） \n请求的 URI（通常为网址）过长，服务器无法处理。";
       break;
    case 415:
       result  = "415   Unsupported Media Type（不支持的媒体类型） \n请求的格式不受请求页面的支持。";
       break;
    case 416:
       result  = "416   Requested Range Not Satisfiable（请求范围不符合要求） \n如果页面无法提供请求的范围，则服务器会返回此状态代码。";
       break;
    case 417:
       result  = "417   Expectation Failed（未满足期望值） \n服务器未满足期望请求标头字段的要求。";
       break;
    case 500:
       result  = "500   Internal Server Error（服务器内部错误）  \n服务器遇到错误，无法完成请求。";
       break;
    case 501:
       result  = "501   Not Implemented（尚未实施） \n服务器不具备完成请求的功能。 例如，服务器无法识别请求方法时可能会返回此代码。";
       break;
    case 504:
       result  = "504   Gateway Timeout（网关超时）  \n服务器作为网关或代理，但是没有及时从上游服务器收到请求。";
       break;
    case 505:
       result  = "505   HTTP Version Not Supported（HTTP 版本不受支持） \n服务器不支持请求中所用的 HTTP 协议版本。";
       break;
    }
    m.insert("StatusCode",QString::number(statusCode));
    m.insert("Result", result);
    return m;
}

QMap<QString, QString> finishReply(QNetworkReply *reply)
{
   QMap<QString, QString> m  = parseStatusCode(reply);
   if(m.value("StatusCode").toInt() == 200)
   {
       if (reply->error() == QNetworkReply::NoError)   //应答是否有错误
       {
           QByteArray bytes(reply->readAll()) ;      //读取所有数据
           QString adxResponse = QString::fromLatin1(bytes);
           m.insert("adxResponse",adxResponse);
       }
   }
   reply->deleteLater();     //释放资源
   return m;
}

bool do_get(QUrl url, QNetworkRequest nr, QMap<QString, QString> &m)
{
     nr.setUrl(url); //设置请求Url
     QNetworkAccessManager nam ;
     QNetworkReply *reply =  nam.get(nr);    //执行Get方法
     QEventLoop loop;
     QObject::connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));   //以get方式发送数据
     loop.exec();
     m = finishReply(reply);
     return true;
}

void do_get(QNetworkAccessManager &manager, QNetworkRequest &req,int num)
{
     for(int i = 0;i < num; ++i)
     {
         manager.get(req);    //执行Get方法
     }
     //QEventLoop loop;
     //QObject::connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));   //以get方式发送数据
     //loop.exec();
}


bool do_post(QUrl url, QString json, QNetworkRequest nr, QMap<QString, QString> &m)
{
    QEventLoop loop;
    QByteArray ba(json.toLatin1());
    QNetworkAccessManager nam ;
    nr.setUrl(url);
    QNetworkReply *reply  =  nam.post(nr, ba);    //执行Post方法;
    QObject::connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));   //以post方式发送数据
    loop.exec();
    m = finishReply(reply);
    reply->deleteLater();
    return true;
}
