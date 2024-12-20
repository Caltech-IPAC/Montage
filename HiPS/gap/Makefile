.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lpixbounds -lwcs -lmtbl -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

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
