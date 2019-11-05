CC=g++
CFLAGS=-O3 -Wall -Wextra -std=gnu++11

all: demo

clop.o:	clop.cpp clop.h
	$(CC) $(CFLAGS) -DCLOP_COMPILE_INFO="\"`date`\"" -c clop.cpp
#	$(CC) $(CFLAGS) -c clop.cpp

demo: demo.cpp clop.o
	$(CC) $(CFLAGS) -o demo demo.cpp clop.o

clean:
	/bin/rm -f *.o 
distclean: clean
	/bin/rm -f *.o demo
build: distclean all

