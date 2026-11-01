.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	cc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lcoord -lmtbl \
		-lboundaries -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mHdr:	mHdr.o montageHdr.o
		$(CC) -o mHdr mHdr.o montageHdr.o $(LIBS)

install:
		cp mHdr ../../bin

clean:
		rm -f mHdr *.o
