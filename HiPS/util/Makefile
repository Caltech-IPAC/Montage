CPPFLAGS += -I. -I../../lib/include -I../../MontageLib -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   = -L../../lib -lcfitsio -lcoord -lwcs -lboundaries -lpixbounds -lmtbl -lsvc -lm

all:   mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine \
       mHPXBgCombine mHPXBgSplit mHPXSplitCorrections mHPXRawHistogram \
       mHPXxy2tile mHPXtile2xy mHPXCombineHist mHPXMap mHPXBlendEdges \
       mHPXCoord2Tile mHPXOrder mHiPSExec


mHiPSExec: mHiPSExec.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXPlateList: mHPXPlateList.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHiPSSetup: mHiPSSetup.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHiPSID: mHiPSID.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHiPSAllSky: mHiPSAllSky.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXPlateCombine: mHPXPlateCombine.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXBgCombine: mHPXBgCombine.o ../../MontageLib/Fitplane/montageFitplane.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXBgSplit: mHPXBgSplit.o ../../MontageLib/Fitplane/montageFitplane.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXSplitCorrections: mHPXSplitCorrections.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXRawHistogram: mHPXRawHistogram.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXxy2tile: mHPXxy2tile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXtile2xy: mHPXtile2xy.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXCombineHist: mHPXCombineHist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXMap: mHPXMap.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXBlendEdges: mHPXBlendEdges.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXCoord2Tile: mHPXCoord2Tile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXOrder: mHPXOrder.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

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
