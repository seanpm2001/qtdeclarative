CONFIG += testcase
TARGET = tst_qquicklistmodelworkerscript
macx:CONFIG -= app_bundle

SOURCES += tst_qquicklistmodelworkerscript.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private v8-private qml-private quick-private testlib
