CC = g++
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = .
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/CDotParseTest

SRC = \
CDotParseTest.cpp

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
-std=c++14 \
-I$(INC_DIR) \
-I. \
-I../../../CFile/include \

LFLAGS = \
-L$(LIB_DIR) \
-L../lib \
-L../../../CFile/lib \
-L../../../CStrUtil/lib \
-L../../../COS/lib \

LIBS = \
-lCGraphViz -lCFile -lCStrUtil -lCOS

clean:
	$(RM) -f *.o
	$(RM) -f CDotParseTest

.SUFFIXES: .cpp

.cpp.o:
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/CDotParseTest: $(OBJS)
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CDotParseTest $(OBJS) $(LFLAGS) $(LIBS)
