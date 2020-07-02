.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CFLAGS =	-g -I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
LIBS   =	-L../../lib -lwcs -lsvc -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mHiPSPNGs:	mHiPSPNGs.o
		$(CC) -o mHiPSPNGs mHiPSPNGs.o $(LIBS)

install:
		cp  mHiPSPNGs ../../bin

clean:
		rm -f mHiPSPNGs *.o
