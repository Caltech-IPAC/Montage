.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mTANHdr:		mTANHdr.o montageTANHdr.o
		$(CC) -o mTANHdr mTANHdr.o montageTANHdr.o ../util/checkHdr.o ../util/checkWCS.o ../util/checkFile.o $(LIBS)

install:
		cp mTANHdr ../../bin

clean:
		rm -f mTANHdr *.o
