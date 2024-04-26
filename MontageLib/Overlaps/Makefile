.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lcoord -lwcs -lmtbl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mOverlaps:	mOverlaps.o montageOverlaps.o
				$(CC) -o mOverlaps mOverlaps.o montageOverlaps.o ../util/debugCheck.o ../util/checkWCS.o $(LIBS)

install:
		cp mOverlaps ../../bin

clean:
		rm -f mOverlaps *.o
