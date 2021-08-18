.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g -O2 -I. -I../../lib/include -I../../MontageLib
LIBS   = -L../../lib -lcfitsio -lwcs -lboundaries -lmtbl -lsvc -lm

.c.o:
	$(CC) $(CFLAGS)  -c  $*.c


all:   mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
       mHPXBgCombine mHPXSplitCorrections mHPXxy2tile mHPXtile2xy


mHPXPlateList: mHPXPlateList.o
	gcc -o mHPXPlateList mHPXPlateList.o $(LIBS)

mHiPSSetup: mHiPSSetup.o
	gcc -o mHiPSSetup mHiPSSetup.o $(LIBS)

mHiPSID: mHiPSID.o
	gcc -o mHiPSID mHiPSID.o $(LIBS)

mHiPSAllSky: mHiPSAllSky.o
	gcc -o mHiPSAllSky mHiPSAllSky.o $(LIBS)

mHPXPlateCombine: mHPXPlateCombine.o
	gcc -o mHPXPlateCombine mHPXPlateCombine.o $(LIBS)

mHPXBgCombine: mHPXBgCombine.o
	gcc -o mHPXBgCombine mHPXBgCombine.o $(LIBS)

mHPXSplitCorrections: mHPXSplitCorrections.o
	gcc -o mHPXSplitCorrections mHPXSplitCorrections.o $(LIBS)

mHPXxy2tile: mHPXxy2tile.o
	gcc -o mHPXxy2tile mHPXxy2tile.o $(LIBS)

mHPXtile2xy: mHPXtile2xy.o
	gcc -o mHPXtile2xy mHPXtile2xy.o $(LIBS)


install:
	cp mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
	mHPXBgCombine mHPXSplitCorrections mHPXxy2tile mHPXtile2xy \
	../../bin

clean:
	rm -f mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
	mHPXBgCombine mHPXSplitCorrections mHPXxy2tile mHPXtile2xy \
	*.o
