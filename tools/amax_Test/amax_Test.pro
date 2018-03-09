#-------------------------------------------------
#
# Project created by QtCreator 2016-05-03T09:07:46
#
#-------------------------------------------------

QT       += widgets
QT       += core gui
QT       += script
QT       += network

TARGET = amax_Test
TEMPLATE = app

SOURCES += main.cpp\
        Amax_Test.cpp \
    Thread.cpp \
    util.cpp \
    common.cpp \
    CopyRequestByPatch_Dialog.cpp \
    SendRequestByPatch_Dialog.cpp \
    SendRequestByParallel_Dialog.cpp

HEADERS  += Amax_Test.h \
    Thread.h \
    util.h \
    type.h \
    common.h \
    CopyRequestByPatch_Dialog.h \
    SendRequestByPatch_Dialog.h \
    SendRequestByParallel_Dialog.h

FORMS    += Amax_Test.ui \
    CopyRequestByPatch_Dialog.ui \
    SendRequestByPatch_Dialog.ui \
    SendRequestByParallel_Dialog.ui



