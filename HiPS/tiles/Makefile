.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CFLAGS =	-g -I. -I../../MontageLib -I../../lib/include \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
LIBS   =	-L../../lib -lwcs -lcfitsio -lpixbounds -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mHiPSTiles mHiPSTileDiffs mTileCleanup

mHiPSTiles:	mHiPSTiles.o
		$(CC) -o mHiPSTiles mHiPSTiles.o $(LIBS)

mTileCleanup:	mTileCleanup.o
		$(CC) -o mTileCleanup mTileCleanup.o $(LIBS)

mHiPSTileDiffs:	mHiPSTileDiffs.o
		$(CC) -o mHiPSTileDiffs mHiPSTileDiffs.o \
	        ../../MontageLib/Fitplane/montageFitplane.o \
	        ../../MontageLib/Background/montageBackground.o \
	        ../../MontageLib/util/checkHdr.o \
	        ../../MontageLib/util/checkWCS.o \
		$(LIBS)

install:
		cp mHiPSTiles mHiPSTileDiffs ../../bin
clean:
		rm -f mHiPSTiles mHiPSTileDiffs *.o
