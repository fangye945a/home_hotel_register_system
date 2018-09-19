#-------------------------------------------------
#
# Project created by QtCreator 2018-09-11T23:05:33
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = homestay
TEMPLATE = app


SOURCES += main.cpp\
        home_hotel.cpp \
    detect_card_pthread.cpp

HEADERS  += home_hotel.h \
    detect_card_pthread.h

FORMS    += home_hotel.ui

RESOURCES += \
    picture.qrc
