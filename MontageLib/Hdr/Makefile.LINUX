.SUFFIXES:
.SUFFIXES: .c .o

CC     =	cc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC
CFLAGS =	-g -I. -I.. -I../../lib/include -Wall
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		-lboundaries -lnsl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mHdr:	mHdr.o montageHdr.o
		$(CC) -o mHdr mHdr.o montageHdr.o $(LIBS)

install:
		cp mHdr ../../bin

clean:
		rm -f mHdr *.o
