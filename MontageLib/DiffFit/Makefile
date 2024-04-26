.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lpixbounds -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mDiffFit:		mDiffFit.o montageDiffFit.o
		$(CC) -o mDiffFit mDiffFit.o montageDiffFit.o \
                            ../Diff/montageDiff.o ../Fitplane/montageFitplane.o \
                            ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o $(LIBS)

install:
		cp mDiffFit ../../bin

clean:
		rm -f mDiffFit *.o
