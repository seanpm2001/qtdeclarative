CONFIG += testcase
TARGET = tst_qquickitem
SOURCES += tst_qquickitem.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

QT += core-private gui-private v8-private qml-private quick-private testlib
