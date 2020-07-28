.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mDiffExec:		mDiffExec.o montageDiffExec.o
		$(CC) -o mDiffExec mDiffExec.o montageDiffExec.o ../Diff/montageDiff.o \
                         ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o $(LIBS)

install:
		cp mDiffExec ../../bin

clean:
		rm -f mDiffExec *.o
