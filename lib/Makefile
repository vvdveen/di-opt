DYNINST_INSTALL ?= $(HOME)/src/dyninst-9.3.2/install
DYNINST_INC_DIR = $(DYNINST_INSTALL)/include
DYNINST_LIB_DIR = $(DYNINST_INSTALL)/lib


CXX      = g++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS  = -ldl -rdynamic -L$(DYNINST_LIB_DIR) -ldyninstAPI 

all: tracer.so

tracer.so: tracer.o
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -I$(DYNINST_INC_DIR) -c -fPIC -o $@ $<

clean:
	rm -f *.o tracer.so

