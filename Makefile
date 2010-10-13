
CC = icc -vec_report0
# CC= gcc
CFLAGS = -w2 -O2  -g -fPIC
# CFLAGS = -Wall -O2  -g -fPIC
CXXFLAGS = $(CFLAGS)
CXX = icpc
# CXX = g++
OPTS = 
PG = 
CFLAGS += $(OPTS)
COM_INC = parameters.h Makefile
LINK=$(CC)
obj=gadgetreader.o read_utils.o
head=read_utils.h gadgetreader.hpp

.PHONY: all clean

all: libgadread.so

libgadread.so: $(obj)
	$(CC) -shared -Wl,-soname,libgadread.so -o libgadread.so   *.o
gadgetreader.o: gadgetreader.cpp $(head) read_utils.o
read_utils.o: read_utils.c read_utils.h
test: PGIIhead libgadread.so btest
PGIIhead: PGIIhead.cpp libgadread.so
btest: btest.cpp
	$(CC) $(CFLAGS) btest.cpp -lboost_unit_test_framework -o btest
clean: 
	rm $(obj) 


