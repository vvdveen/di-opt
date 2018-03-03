DYNINST_INSTALL ?= $(HOME)/src/dyninst-9.3.2/install
DYNINST_BIN_DIR = $(DYNINST_INSTALL)/bin
DYNINST_INC_DIR = $(DYNINST_INSTALL)/include
DYNINST_LIB_DIR = $(DYNINST_INSTALL)/lib

CXX      = g++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS  = -ldl -rdynamic -L$(DYNINST_LIB_DIR) -ldyninstAPI 

HEADERS=$(wildcard *.h)
INSTALLED_HEADERS=$(patsubst %.h,$(DYNINST_INC_DIR)/%.h,$(HEADERS))

all: di-opt 

di-opt: di-opt.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(DYNINST_INC_DIR) -c -o $@ $<

clean:
	rm -f *.o di-opt

.PHONY: install
install: di-opt
	cp $< $(DYNINST_BIN_DIR)
	cp $(HEADERS) $(DYNINST_INC_DIR)

.PHONY: uninstall
uninstall:
	rm -f $(DYNINST_BIN_DIR)/di-opt
	rm -f $(INSTALLED_HEADERS)
