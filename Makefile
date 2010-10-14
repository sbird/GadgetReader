
# CC = icc -vec_report0
CC= gcc
# CFLAGS = -w2 -O2  -g -fPIC
CFLAGS = -Wall -O2  -g -fPIC
CXXFLAGS = $(CFLAGS)
# CXX = icpc
CXX = g++
OPTS = 
PG = 
CFLAGS += $(OPTS)
obj=gadgetreader.o read_utils.o
head=read_utils.h gadgetreader.hpp

.PHONY: all clean test

all: libgadread.so

libgadread.so: $(obj)
	$(CC) -shared -Wl,-soname,$@ -o $@  $(obj)
gadgetreader.o: gadgetreader.cpp $(head) read_utils.o
read_utils.o: read_utils.c read_utils.h
test: PGIIhead btest 
	@./btest
	@./PGIIhead test_g2_snap 1 > PGIIhead_out.test 2>/dev/null
	@echo "Errors in PGIIhead output:"
	@diff PGIIhead_out.test PGIIhead_out.txt
PGIIhead: PGIIhead.cpp libgadread.so
btest: btest.cpp libgadread.so
	$(CC) $(CFLAGS) $< -lboost_unit_test_framework -lgadread -L. -o $@

clean: 
	rm $(obj) PGIIhead btest


