.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lpixbounds -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mDiffFitExec:		mDiffFitExec.o montageDiffFitExec.o
		$(CC) -o mDiffFitExec mDiffFitExec.o montageDiffFitExec.o \
                            ../Diff/montageDiff.o ../Fitplane/montageFitplane.o \
                            ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o $(LIBS)

install:
		cp mDiffFitExec ../../bin

clean:
		rm -f mDiffFitExec *.o
