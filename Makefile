#Comment this if you don't need HDF5
OPTS = -DHAVE_HDF5
#Comment this if you don't need bigfile output
OPTS += -DHAVE_BIGFILE
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
  CFLAGS += -Wall -O2  -g -fPIC -std=gnu++11
endif
CXXFLAGS += $(CFLAGS)
LDFLAGS +=-Wl,-rpath,${CURDIR},--no-add-needed,--as-needed -L${CURDIR} -lrgad
ifeq (HAVE_HDF5,$(findstring HAVE_HDF5,${OPTS}))
	HDF_LINK = -lhdf5 -lhdf5_hl
else
	HDF_LINK =
endif
ifeq (HAVE_BIGFILE,$(findstring HAVE_BIGFILE,${OPTS}))
	BGFL_LINK = -Lbigfile/lib -lbigfile 
	BGFL_INC = -Ibigfile/include
else
	BGFL_LINK =
endif

PG = 
CFLAGS += $(OPTS) $(BGFL_INC)
obj=gadgetreader.o
head=read_utils.h gadgetreader.hpp gadgetheader.h
#Include directories for python and perl.
PYINC:=$(shell python-config --includes)
#Check python-config isn't a python 3 version: 
ifeq (python3,$(findstring python3,${PYINC}))
	PYINC:=$(shell python2-config --includes)
endif
PERLINC=-I/usr/lib/perl5/core_perl/CORE

.PHONY: all clean test dist pybind bind

all: librgad.so libwgad.so libwbfgad.so PGIIhead PosDump Convert2HDF5

librgad.so: librgad.so.1
	ln -sf $< $@

librgad.so.1: $(obj)
	$(CC) -shared -Wl,-soname,$@ -o $@  $^

libwbfgad.so: libwbfgad.so.0
	ln -sf $< $@

#Writer library.
libwgad.so: libwgad.so.1
	ln -sf $< $@

libwgad.so.1: gadgetwriter.o gadgetwritehdf.o gadgetwriteoldgadget.o
	$(CC) -shared -Wl,-soname,$@ $(HDF_LINK) -o $@ $^

%.o: %.cpp gadgetwritefile.hpp

libwbfgad.so.0: gadgetwritebigfile.cpp gadgetwritebigfile.hpp gadgetheader.h
	mpic++ $(CFLAGS) -shared -Wl,-soname,$@ -o $@  $^

gadgetreader.o: gadgetreader.cpp $(head)
gadgetwriter.o: gadgetwriter.cpp gadgetwriter.hpp gadgetheader.h gadgetwritefile.hpp

test: PGIIhead btest 
	@./btest
	@./PGIIhead test_g2_snap 1 > PGIIhead_out.test 2>/dev/null
	@echo "Any errors in PGIIhead output will be printed below:"
	@diff PGIIhead_out.test PGIIhead_out.txt
PGIIhead: PGIIhead.cpp librgad.so
PosDump: PosDump.cpp librgad.so
Convert2HDF5: Convert2HDF5.cpp librgad.so libwgad.so
	${CXX} $(CFLAGS) $< ${LDFLAGS} -lwgad -o $@

btest: btest.cpp librgad.so
	$(CXX) $(CFLAGS) $< ${LDFLAGS} -lboost_unit_test_framework -o $@

clean: 
	-rm -f $(obj) gadgetwriter.o PGIIhead PosDump btest librgad.so librgad.so.1 libwgad.so libwgad.so.1 libwbfgad.so libwbfgad.so.0
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

dist: Makefile README $(head) Doxyfile PGIIhead.cpp PGIIhead_out.txt btest.cpp gadgetreader.cpp gadgetreader.i test_g2_snap.0 test_g2_snap.1 PosDump.cpp gadgetwriter.cpp gadgetwriter.hpp
	tar -czf GadgetReader.tar.gz $^
