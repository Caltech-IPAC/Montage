.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lboundaries -lmtbl -lcoord -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mMakeHdr:	mMakeHdr.o montageMakeHdr.o
				$(CC) -o mMakeHdr mMakeHdr.o montageMakeHdr.o ../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o $(LIBS)

install:
		cp mMakeHdr ../../bin

clean:
		rm -f mMakeHdr *.o
