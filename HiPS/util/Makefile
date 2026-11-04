.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g -O2 -I. -I../../lib/include -I../../MontageLib
LIBS   = -L../../lib -lcfitsio -lcoord -lwcs -lboundaries -lpixbounds -lmtbl -lsvc -lm

.c.o:
	$(CC) $(CFLAGS)  -c  $*.c


all:   mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
       mHPXBgCombine mHPXBgSplit mHPXSplitCorrections mHPXRawHistogram \
       mHPXxy2tile mHPXtile2xy mHPXCombineHist mHPXMap mHPXBlendEdges \
       mHPXCoord2Tile mHPXOrder mHiPSExec


mHiPSExec: mHiPSExec.o
	gcc -o mHiPSExec mHiPSExec.o $(LIBS)

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
	gcc -o mHPXBgCombine mHPXBgCombine.o ../../MontageLib/Fitplane/montageFitplane.o $(LIBS)

mHPXBgSplit: mHPXBgSplit.o
	gcc -o mHPXBgSplit mHPXBgSplit.o ../../MontageLib/Fitplane/montageFitplane.o $(LIBS)

mHPXSplitCorrections: mHPXSplitCorrections.o
	gcc -o mHPXSplitCorrections mHPXSplitCorrections.o $(LIBS)

mHPXRawHistogram: mHPXRawHistogram.o
	gcc -o mHPXRawHistogram mHPXRawHistogram.o $(LIBS)

mHPXxy2tile: mHPXxy2tile.o
	gcc -o mHPXxy2tile mHPXxy2tile.o $(LIBS)

mHPXtile2xy: mHPXtile2xy.o
	gcc -o mHPXtile2xy mHPXtile2xy.o $(LIBS)

mHPXCombineHist: mHPXCombineHist.o
	gcc -o mHPXCombineHist mHPXCombineHist.o $(LIBS)

mHPXMap: mHPXMap.o
	gcc -o mHPXMap mHPXMap.o $(LIBS)

mHPXBlendEdges: mHPXBlendEdges.o
	gcc -o mHPXBlendEdges mHPXBlendEdges.o $(LIBS)

mHPXCoord2Tile: mHPXCoord2Tile.o
	gcc -o mHPXCoord2Tile mHPXCoord2Tile.o $(LIBS)

mHPXOrder: mHPXOrder.o
	gcc -o mHPXOrder mHPXOrder.o $(LIBS)


install:
	cp mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
	mHPXBgCombine mHPXBgSplit mHPXSplitCorrections mHPXRawHistogram \
	mHPXxy2tile mHPXtile2xy mHPXCombineHist mHPXMap mHPXBlendEdges \
	mHPXCoord2Tile mHPXOrder mHiPSExec ../../bin

clean:
	rm -f mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
	mHPXBgCombine mHPXBgSplit mHPXSplitCorrections mHPXRawHistogram \
	mHPXxy2tile mHPXtile2xy mHPXCombineHist mHPXMap mHPXBlendEdges \
	mHPXCoord2Tile mHPXOrder mHiPSExec *.o
