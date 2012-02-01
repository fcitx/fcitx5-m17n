CFLAGS := $(shell m17n-config --cflags)
CFLAGS := $(CFLAGS) -fPIC -g
CXXFLAGS= $(CFLAGS) -std=c++11
LDFLAGS :=
M17NLIBS := $(shell m17n-config --libs)
OUTFILES := testmim

include ../common.mk
include ../config.mk
include config.mk

testmim.o: m17n.hh

testmim: testmim.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(M17NLIBS)

all: testmim

