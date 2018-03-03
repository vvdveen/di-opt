DYNINST_INSTALL ?= $(HOME)/src/dyninst-9.3.2/install
DYNINST_INC_DIR = $(DYNINST_INSTALL)/include
DYNINST_LIB_DIR = $(DYNINST_INSTALL)/lib


CXX      = g++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS  = -ldl -rdynamic -L$(DYNINST_LIB_DIR) -ldyninstAPI 

SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRCS))

all: libplugins.so

libplugins.so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(DYNINST_INC_DIR) -c -fPIC -o $@ $<

clean:
	rm -f *.o libplugins.so

