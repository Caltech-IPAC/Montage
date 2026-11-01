.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lpixbounds -lwcs -lmtbl $(CFITSIO_LIBS) -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mHPXGap mHPXGapDiff mHPXGapCombine

mHPXGap:	mHPXGapScripts.o
		$(CC) -o mHPXGapScripts mHPXGapScripts.o ../../MontageLib/util/debugCheck.o $(LIBS)

mHPXGapDiff:	mHPXGapDiff.o montageHPXGapDiff.o
		$(CC) -o mHPXGapDiff mHPXGapDiff.o montageHPXGapDiff.o ../../MontageLib/util/debugCheck.o $(LIBS)

mHPXGapCombine:	mHPXGapCombine.o
		$(CC) -o mHPXGapCombine mHPXGapCombine.o ../../MontageLib/util/debugCheck.o $(LIBS)

install:
		cp mHPXGapScripts mHPXGapDiff mHPXGapCombine ../../bin

clean:
		rm -f mHPXGapScripts mHPXGapDiff mHPXGapCombine *.o
