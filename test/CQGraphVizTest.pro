TEMPLATE = app

QT += widgets

TARGET = CQGraphVizTest

DEPENDPATH += .

MOC_DIR = .moc

QMAKE_CXXFLAGS += -std=c++14

CONFIG += debug

SOURCES += \
CQGraphVizTest.cpp \

HEADERS += \
CQGraphVizTest.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

PRE_TARGETDEPS = \
$(LIB_DIR)/libCQGraphViz.a \

INCLUDEPATH = \
. \
../include \
../../CUtil/include \
../../CMath/include \
../../COS/include \

unix:LIBS += \
-L$$LIB_DIR \
-L../../CQGraphViz/lib \
-L../../CQDot/graphviz/lib \
-L../../CJson/lib \
-L../../CUtil/lib \
-L../../CFile/lib \
-L../../CStrUtil/lib \
-L../../COS/lib \
\
-lCQGraphViz -lCGraphViz -lCJson \
-lCFile -lCUtil -lCStrUtil -lCOS
