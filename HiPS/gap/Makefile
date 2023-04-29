.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lpixbounds -lwcs -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mHPXGap mHPXGapDiff

mHPXGap:	mHPXGapScripts.o
		$(CC) -o mHPXGapScripts mHPXGapScripts.o ../../MontageLib/util/debugCheck.o $(LIBS)

mHPXGapDiff:	mHPXGapDiff.o montageHPXGapDiff.o
		$(CC) -o mHPXGapDiff mHPXGapDiff.o montageHPXGapDiff.o ../../MontageLib/util/debugCheck.o $(LIBS)

install:
		cp mHPXGapScripts mHPXGapDiff ../../bin

clean:
		rm -f mHPXGapScripts mHPXGapDiff *.o
