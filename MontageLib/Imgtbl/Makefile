.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lcoord -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mImgtbl:		mImgtbl.o montageImgtbl.o
		$(CC) -o mImgtbl mImgtbl.o montageImgtbl.o ../util/checkWCS.o $(LIBS)

install:
		cp mImgtbl ../../bin

clean:
		rm -f mImgtbl *.o
