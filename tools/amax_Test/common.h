#ifndef COMMON
#define COMMON

#include <QUrl>
#include <QMap>
#include <QString>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QObject>
#include <QTextStream>

bool writeFile(QWidget *widget, const char *filter, QString dir, QIODevice::OpenMode mode, QFile *&outFile, QTextStream *&out);

bool readFile(QWidget *widget, const char *filter, QString dir, QIODevice::OpenMode mode, QFile *&inFile, QTextStream *&in);

QMap<QString,QString> parseStatusCode(QNetworkReply *reply);

QMap<QString, QString> finishReply(QNetworkReply *reply);

bool isJson(QString& json);

bool isUrl(QUrl& url);

QString formatText(QString json);

bool do_get(QUrl url, QNetworkRequest nr, QMap<QString, QString> &m);
void do_get(QNetworkAccessManager &manager, QNetworkRequest &req,int num);

bool do_post(QUrl url, QString json, QNetworkRequest nr, QMap<QString, QString> &m);

#endif // COMMON

