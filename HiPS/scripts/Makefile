.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc
CPPFLAGS += -I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   = -L../../lib -lwcs -lboundaries -lmtbl -lsvc -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c


all:      mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgCorrectScripts mHPXCoaddScripts \
          mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts mHPXQuicklookScripts \
	  mHPXBlendScripts mHPXHistogramScripts mHPXCombineHistScripts mHPXPlatePNGScripts \
	  mHPXFlattenScripts mHPX8BitScripts mHPXColorQuicklookScripts

mHPXMosaicScripts: mHPXMosaicScripts.o
			$(CC) $(LDFLAGS) -o mHPXMosaicScripts mHPXMosaicScripts.o $(LIBS)

mHPXProjectScripts: mHPXProjectScripts.o
			$(CC) $(LDFLAGS) -o mHPXProjectScripts mHPXProjectScripts.o $(LIBS)

mHPXDiffScripts: mHPXDiffScripts.o
			$(CC) $(LDFLAGS) -o mHPXDiffScripts mHPXDiffScripts.o $(LIBS)

mHPXBgCorrectScripts: mHPXBgCorrectScripts.o
			$(CC) $(LDFLAGS) -o mHPXBgCorrectScripts mHPXBgCorrectScripts.o $(LIBS)

mHPXCoaddScripts: mHPXCoaddScripts.o
			$(CC) $(LDFLAGS) -o mHPXCoaddScripts mHPXCoaddScripts.o $(LIBS)

mHPXShrinkScripts: mHPXShrinkScripts.o
			$(CC) $(LDFLAGS) -o mHPXShrinkScripts mHPXShrinkScripts.o $(LIBS)

mHPXQuicklookScripts: mHPXQuicklookScripts.o
			$(CC) $(LDFLAGS) -o mHPXQuicklookScripts mHPXQuicklookScripts.o $(LIBS)

mHiPSTileScripts: mHiPSTileScripts.o
			$(CC) $(LDFLAGS) -o mHiPSTileScripts mHiPSTileScripts.o $(LIBS)

mHiPSPNGScripts: mHiPSPNGScripts.o
			$(CC) $(LDFLAGS) -o mHiPSPNGScripts mHiPSPNGScripts.o $(LIBS)

mHPXBlendScripts: mHPXBlendScripts.o
			$(CC) $(LDFLAGS) -o mHPXBlendScripts mHPXBlendScripts.o $(LIBS)

mHPXHistogramScripts: mHPXHistogramScripts.o
			$(CC) $(LDFLAGS) -o mHPXHistogramScripts mHPXHistogramScripts.o $(LIBS)

mHPX8BitScripts: mHPX8BitScripts.o
			$(CC) $(LDFLAGS) -o mHPX8BitScripts mHPX8BitScripts.o $(LIBS)

mHPXCombineHistScripts: mHPXCombineHistScripts.o
			$(CC) $(LDFLAGS) -o mHPXCombineHistScripts mHPXCombineHistScripts.o $(LIBS)

mHPXPlatePNGScripts: mHPXPlatePNGScripts.o
			$(CC) $(LDFLAGS) -o mHPXPlatePNGScripts mHPXPlatePNGScripts.o $(LIBS)

mHPXColorQuicklookScripts: mHPXColorQuicklookScripts.o
			$(CC) $(LDFLAGS) -o mHPXColorQuicklookScripts mHPXColorQuicklookScripts.o $(LIBS)

mHPXFlattenScripts: mHPXFlattenScripts.o
			$(CC) $(LDFLAGS) -o mHPXFlattenScripts mHPXFlattenScripts.o $(LIBS)

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
