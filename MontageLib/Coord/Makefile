.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lcoord -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mCoord:	mCoord.o montageCoord.o
		$(CC) -o mCoord mCoord.o montageCoord.o \
		../util/checkHdr.o   \
		../util/checkWCS.o   \
		$(LIBS)

install:
		cp mCoord ../../bin

clean:
		rm -f mCoord *.o
