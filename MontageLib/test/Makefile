.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lcmd -lsvc -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mTest:		mTest.o
		$(CC) -o mTest mTest.o $(LIBS)

install:
		cp mTest ../../bin

clean:
		rm -f mTest *.o
