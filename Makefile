
# CC = icc -vec_report0
CC= gcc
# CFLAGS = -w2 -O2  -g -fPIC
CFLAGS = -Wall -O2  -g -fPIC
CXXFLAGS = $(CFLAGS)
# CXX = icpc
CXX = g++
LDFLAGS=-Wl,-rpath,. -L. -lrgad
OPTS = 
PG = 
CFLAGS += $(OPTS)
obj=gadgetreader.o read_utils.o
head=read_utils.h gadgetreader.hpp
#Include directories for python and perl
PYINC=/usr/include/python2.6
PERLINC=/usr/lib/perl5/core_perl/CORE

.PHONY: all clean test dist pybind bind

all: librgad.so

librgad.so: librgad.so.1
	ln -sf $< $@

librgad.so.1: $(obj)
	$(CC) -shared -Wl,-soname,$@ -o $@  $(obj)
gadgetreader.o: gadgetreader.cpp $(head) read_utils.o
read_utils.o: read_utils.c read_utils.h
test: PGIIhead btest 
	@./btest
	@./PGIIhead test_g2_snap 1 > PGIIhead_out.test 2>/dev/null
	@echo "Any errors in PGIIhead output will be printed below:"
	@diff PGIIhead_out.test PGIIhead_out.txt
PGIIhead: PGIIhead.cpp librgad.so
btest: btest.cpp librgad.so
	$(CC) $(CFLAGS) $< -lboost_unit_test_framework ${LDFLAGS} -o $@

clean: 
	rm $(obj) PGIIhead btest librgad.so librgad.so.1

dist:
	tar -czf GadgetReader.tar.gz Makefile $(head) *.cpp *.c test_g2_snap.*

bind: pybind

python:
	mkdir python
perl:
	mkdir perl

pybind: gadgetreader.i librgad.so python
	swig -Wall -python -c++ -o python/gadgetreader_python.cxx $< 
	$(CXX) ${CXXFLAGS} -I${PYINC} -shared -Wl,-soname,_gadgetreader.so -L. -lgadread python/gadgetreader_python.cxx -o python/_gadgetreader.so 

#WARNING: Not as functional as python bindings
perlbind: gadgetreader.i librgad.so perl 
	swig -Wall -perl -c++ -o perl/gadgetreader_perl.cxx $< 
	$(CXX) ${CXXFLAGS} -I${PERLINC} -shared -Wl,-soname,_gadgetreader.so -L. -lgadread perl/gadgetreader_perl.cxx -o perl/_gadgetreader.so 
