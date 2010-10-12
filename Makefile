
CC = icc -openmp -vec_report0
#CC= gcc -fopenmp -Wall 
CXXFLAGS = -Wall -w2 -O2  -g
CXX = icpc
OPTS = 
PG = 
CFLAGS += $(OPTS)
COM_INC = parameters.h Makefile
LINK=$(CC)
#LINK=$(CC) -lm -lgomp -lsrfftw -lsfftw  -L$(FFTW)
obj=gadgetreader.o read_utils.o
head=read_utils.h gadgetreader.hpp

.PHONY: all clean

all: $(obj)


gadgetreader.o: gadgetreader.cpp $(head) read_utils.o
read_utils.o: read_utils.c read_utils.h
test: test.c libgadread.o

clean: 
	rm $(obj) 


