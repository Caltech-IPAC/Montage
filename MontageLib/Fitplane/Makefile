.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lpixbounds -lwcs -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mFitplane:		mFitplane.o montageFitplane.o
		$(CC) -o mFitplane mFitplane.o montageFitplane.o ../util/debugCheck.o ../util/checkHdr.o ../util/checkWCS.o $(LIBS)

install:
		cp mFitplane ../../bin

clean:
		rm -f mFitplane *.o
