#Comment this if you don't need HDF5
OPTS = -DHAVE_HDF5
#Comment this if you don't need bigfile
OPTS += -DHAVE_BGFL
#Use this if you want to write with the bigfile MPI routines.
#OPTS += -DBIGFILE_MPI

#Are we using gcc or icc?
CFLAGS += -Wall -O2  -g -fPIC -std=gnu++11
CXXFLAGS += $(CFLAGS)
LDFLAGS += -L${CURDIR} -lrgad
#Mac's ld doesn't use -soname or -shared, so check for it.
#We are looking for the response: ld unknown option: -soname
LDCHECK:=$(shell ld -soname 2>&1)
ifneq (unknown,$(findstring unknown,${LDCHECK}))
	LDFLAGS +=-Wl,-rpath,${CURDIR},--no-add-needed,--as-needed
	#compile flags for the libraries
	LIBFLAGS +=-shared -Wl,-soname,$@,--no-add-needed,--as-needed
else
	LIBFLAGS += -dynamiclib -Wl,-install_name,$@
endif
#Linker to use
LINK = $(CXX)

HDF_INC =
ifeq (HAVE_HDF5,$(findstring HAVE_HDF5,${OPTS}))
	#Check for a pkgconfig; if one exists we are probably debian.
	ifeq ($(shell pkg-config --exists hdf5-serial && echo 1),1)
		HDF_LINK = $(shell pkg-config --libs hdf5-serial) -lhdf5_hl
		HDF_INC = $(shell pkg-config --cflags hdf5-serial)
	else
		HDF_LINK = -lhdf5 -lhdf5_hl
	endif
else
	HDF_LINK =
endif
ifeq (HAVE_BGFL,$(findstring HAVE_BGFL,${OPTS}))
	BGFL_LINK = -Lbigfile/src -lbigfile
ifeq (BIGFILE_MPI,$(findstring BIGFILE_MPI,${OPTS}))
   	BGFL_LINK += -lbigfile-mpi
	LINK=mpic++
	LIBBIGFILE = libbigfile-mpi.a
else
	LIBBIGFILE = libbigfile.a
endif
	BGFL_INC = -Isubprojects/bigfile/src
else
	BGFL_LINK =
	BGFL_INC =
endif

PG = 
CFLAGS += $(OPTS) $(BGFL_INC) $(HDF_INC)
obj=gadgetreader.o
head=read_utils.h gadgetreader.hpp gadgetheader.h
.PHONY: all clean test dist

all: librgad.so libwgad.so PGIIhead PosDump Convert2HDF5

librgad.so: librgad.so.1
	ln -sf $< $@

librgad.so.1: $(obj)
	$(CXX) $^ $(LIBFLAGS) -o $@

#Writer library.
libwgad.so: libwgad.so.1
	ln -sf $< $@

$(LIBBIGFILE):
	cd $(CURDIR)/subprojects/bigfile/src; VPATH=$(CURDIR)/subprojects/bigfile/src; MPICC=$(CC) make $@

libwgad.so.1: gadgetwriter.o gadgetwritehdf.o gadgetwriteoldgadget.o gadgetwritebigfile.o $(LIBBIGFILE)
	$(LINK) $(filter-out $(LIBBIGFILE),$^) $(LIBFLAGS) $(HDF_LINK) -o $@ $(BGFL_LINK)

gadgetwritebigfile.o: gadgetwritebigfile.cpp gadgetwritebigfile.hpp
	$(LINK) $(CFLAGS) -c $^

%.o: %.cpp %.hpp gadgetheader.h gadgetwritefile.hpp

gadgetreader.o: gadgetreader.cpp $(head)

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
	-rm *.o PGIIhead PosDump btest librgad.so librgad.so.1 libwgad.so libwgad.so.1
cleanall: clean
	-rm -Rf python perl doc

doc: Doxyfile gadgetreader.hpp gadgetreader.cpp
	doxygen $<

dist: Makefile README $(head) Doxyfile PGIIhead.cpp PGIIhead_out.txt btest.cpp gadgetreader.cpp gadgetreader.i test_g2_snap.0 test_g2_snap.1 PosDump.cpp gadgetwriter.cpp gadgetwriter.hpp
	tar -czf GadgetReader.tar.gz $^
