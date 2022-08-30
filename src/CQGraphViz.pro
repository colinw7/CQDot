TEMPLATE = lib

TARGET = CQGraphViz

DEPENDPATH += .

QT += widgets

CONFIG += staticlib

QMAKE_CXXFLAGS += -std=c++17

MOC_DIR = .moc

SOURCES += \
CQGraphViz.cpp \

HEADERS += \
../include/CQGraphViz.h \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += \
../include \
../../CJson/include \
../../CFile/include \
../../CStrUtil/include \
../graphviz/include \
