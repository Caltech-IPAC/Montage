.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mBgExec:		mBgExec.o montageBgExec.o
		$(CC) -o mBgExec mBgExec.o montageBgExec.o ../Background/montageBackground.o \
                       ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o $(LIBS)

install:
		cp mBgExec ../../bin

clean:
		rm -f mBgExec *.o
