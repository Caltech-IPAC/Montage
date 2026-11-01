.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs -lcoord -lmtbl $(CFITSIO_LIBS) -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mImgtbl:		mImgtbl.o montageImgtbl.o
		$(CC) -o mImgtbl mImgtbl.o montageImgtbl.o ../util/checkWCS.o $(LIBS)

install:
		cp mImgtbl ../../bin

clean:
		rm -f mImgtbl *.o
