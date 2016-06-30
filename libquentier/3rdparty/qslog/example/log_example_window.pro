QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = log_example_window
TEMPLATE = app

DEFINES += QS_LOG_WINDOW
include(../QsLog.pri)

SOURCES += log_example_window_main.cpp
