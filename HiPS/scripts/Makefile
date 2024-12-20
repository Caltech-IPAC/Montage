CPPFLAGS += -I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   = -L../../lib -lwcs -lboundaries -lmtbl -lsvc -lcfitsio -lm

all:      mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgCorrectScripts mHPXCoaddScripts \
          mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts mHPXQuicklookScripts \
	  mHPXBlendScripts mHPXHistogramScripts mHPXCombineHistScripts mHPXPlatePNGScripts \
	  mHPXFlattenScripts mHPX8BitScripts mHPXColorQuicklookScripts

mHPXMosaicScripts: mHPXMosaicScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXProjectScripts: mHPXProjectScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXDiffScripts: mHPXDiffScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXBgCorrectScripts: mHPXBgCorrectScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXCoaddScripts: mHPXCoaddScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXShrinkScripts: mHPXShrinkScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXQuicklookScripts: mHPXQuicklookScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHiPSTileScripts: mHiPSTileScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHiPSPNGScripts: mHiPSPNGScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXBlendScripts: mHPXBlendScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXHistogramScripts: mHPXHistogramScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPX8BitScripts: mHPX8BitScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXCombineHistScripts: mHPXCombineHistScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXPlatePNGScripts: mHPXPlatePNGScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXColorQuicklookScripts: mHPXColorQuicklookScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXFlattenScripts: mHPXFlattenScripts.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
			cp mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgCorrectScripts mHPXCoaddScripts \
			   mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts mHPXQuicklookScripts mHPXBlendScripts \
			   mHPXHistogramScripts mHPXCombineHistScripts mHPXPlatePNGScripts mHPXFlattenScripts \
			   mHPX8BitScripts mHPXColorQuicklookScripts ../../bin

clean:
			rm -f mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgCorrectScripts mHPXCoaddScripts \
			   mHPXShrinkScripts mHPXQuicklookScripts mHiPSTileScripts mHiPSPNGScripts mHPXBlendScripts \
			   mHPXHistogramScripts mHPXCombineHistScripts mHPXPlatePNGScripts mHPXFlattenScripts \
			   mHPX8BitScripts mHPXColorQuicklookScripts *.o
