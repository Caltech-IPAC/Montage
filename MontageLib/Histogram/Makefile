.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mHistogram 

mHistogram:	mHistogram.o montageHistogram.o
		$(CC) -o mHistogram mHistogram.o montageHistogram.o $(LIBS)

install:
		cp mHistogram ../../bin

clean:
		rm -f mHistogram *.o
