#-------------------------------------------------
#
# Project created by QtCreator 2016-10-13T10:36:17
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = client_test
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    type.h

FORMS    += mainwindow.ui
LIBS     += -lWs2_32

CONFIG += qaxcontainer

