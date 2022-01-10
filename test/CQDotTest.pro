TEMPLATE = app

QT += widgets

TARGET = CQDotTest

DEPENDPATH += .

MOC_DIR = .moc

QMAKE_CXXFLAGS += -std=c++14

CONFIG += debug

SOURCES += \
CQDotTest.cpp \

HEADERS += \
CQDotTest.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

PRE_TARGETDEPS = \
$(LIB_DIR)/libCQDot.a \

INCLUDEPATH = \
. \
../include \
../../CUtil/include \
../../CMath/include \
../../COS/include \

unix:LIBS += \
-L$$LIB_DIR \
-L../../CQDot/lib \
-L../../CJson/lib \
-L../../CUtil/lib \
-L../../CStrUtil/lib \
\
-lCQDot -lCJson -lCUtil -lCStrUtil \
