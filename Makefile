ifeq ($(CC),cc)
  ICC:=$(shell which icc --tty-only 2>&1)
  #Can we find icc?
  ifeq (/icc,$(findstring /icc,${ICC}))
     CC = icc -vec_report0
     CXX = icpc
  else
     GCC:=$(shell which gcc --tty-only 2>&1)
     #Can we find gcc?
     ifeq (/gcc,$(findstring /gcc,${GCC}))
        CC = gcc
        CXX = g++
     endif
  endif
endif

#Are we using gcc or icc?
ifeq (icc,$(findstring icc,${CC}))
  CFLAGS += -w1 -O2  -g -fPIC
else
  CFLAGS += -Wall -O2  -g -fPIC
endif
CXXFLAGS += $(CFLAGS)
LDFLAGS +=-Wl,-rpath,${CURDIR} -L${CURDIR} -lrgad
OPTS = 
PG = 
CFLAGS += $(OPTS)
obj=gadgetreader.o read_utils.o
head=read_utils.h gadgetreader.hpp gadgetheader.h
#Include directories for python and perl.
PYINC:=$(shell python-config --includes)
#Check python-config isn't a python 3 version: 
ifeq (python3,$(findstring python3,${PYINC}))
	PYINC:=$(shell python2-config --includes)
endif
PERLINC=-I/usr/lib/perl5/core_perl/CORE

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
	-rm -f $(obj) PGIIhead btest librgad.so librgad.so.1
cleanall: clean
	-rm -Rf python perl doc

doc: Doxyfile gadgetreader.hpp gadgetreader.cpp
	doxygen $<

bind: pybind

python:
	mkdir python
perl:
	mkdir perl

python/rgad_python.cxx: gadgetreader.i python
	swig -Wall -python -c++ -o $@ $<

python/_gadgetreader.so: python/rgad_python.cxx librgad.so python
	$(CXX) ${CXXFLAGS} ${PYINC} -shared -Wl,-soname,_gadgetreader.so ${LDFLAGS} $< -o $@

pybind: python/_gadgetreader.so

#WARNING: Not as functional as python bindings
perl/rgad_perl.cxx: gadgetreader.i perl
	swig -Wall -perl -c++ -o $@ $<

perl/_gadgetreader.so: perl/rgad_perl.cxx librgad.so perl
	$(CXX) ${CXXFLAGS} ${PERLINC} -shared -Wl,-soname,_gadgetreader.so ${LDFLAGS} $< -o $@

perlbind: perl/_gadgetreader.so

dist: Makefile README $(head) Doxyfile PGIIhead.cpp PGIIhead_out.txt btest.cpp read_utils.c gadgetreader.cpp gadgetreader.i test_g2_snap.0 test_g2_snap.1
	tar -czf GadgetReader.tar.gz $^
