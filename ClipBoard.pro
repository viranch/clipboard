#-------------------------------------------------
#
# Project created by QtCreator 2011-06-28T13:58:59
#
#-------------------------------------------------

QT       += core gui network

TARGET = ClipBoard
TEMPLATE = app


SOURCES += main.cpp\
        clipboard.cpp \
    messagedialog.cpp \
    settingsdialog.cpp

HEADERS  += clipboard.h \
    messagedialog.h \
    settingsdialog.h

FORMS    += clipboard.ui \
    messagedialog.ui \
    settingsdialog.ui

RESOURCES += \
    icons.qrc
