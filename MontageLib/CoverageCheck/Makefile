.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lboundaries -lmtbl -lcoord -lwcs -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mCoverageCheck:	mCoverageCheck.o montageCoverageCheck.o
						$(CC) -o mCoverageCheck mCoverageCheck.o montageCoverageCheck.o ../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o $(LIBS)

install:
		cp mCoverageCheck ../../bin

clean:
		rm -f mCoverageCheck *.o
