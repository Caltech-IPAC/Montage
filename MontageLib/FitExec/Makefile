.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lpixbounds -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mFitExec:		mFitExec.o montageFitExec.o
		$(CC) -o mFitExec mFitExec.o montageFitExec.o ../Fitplane/montageFitplane.o \
                        ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o ../util/checkFile.o $(LIBS)

install:
		cp mFitExec ../../bin

clean:
		rm -f mFitExec *.o
