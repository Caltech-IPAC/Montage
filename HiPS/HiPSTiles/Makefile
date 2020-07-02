.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CFLAGS =	-g -I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
LIBS   =	-L../../lib -lwcs -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mHiPSTiles:	mHiPSTiles.o
		$(CC) -o mHiPSTiles mHiPSTiles.o $(LIBS)

install:
		cp mHiPSTiles ../../bin
clean:
		rm -f mHiPSTiles *.o
