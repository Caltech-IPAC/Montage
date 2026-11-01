.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g -O2 -I. -I../../lib/include
LIBS   = -L../../lib -lwcs -lboundaries -lmtbl -lsvc -lcfitsio -lm

.c.o:
			$(CC) $(CFLAGS)  -c  $*.c


all:      mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgCorrectScripts mHPXCoaddScripts \
          mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts mHPXQuicklookScripts \
	  mHPXBlendScripts mHPXHistogramScripts mHPXCombineHistScripts mHPXPlatePNGScripts \
	  mHPXFlattenScripts mHPX8BitScripts mHPXColorQuicklookScripts

mHPXMosaicScripts: mHPXMosaicScripts.o
			gcc -o mHPXMosaicScripts mHPXMosaicScripts.o $(LIBS)

mHPXProjectScripts: mHPXProjectScripts.o
			gcc -o mHPXProjectScripts mHPXProjectScripts.o $(LIBS)

mHPXDiffScripts: mHPXDiffScripts.o
			gcc -o mHPXDiffScripts mHPXDiffScripts.o $(LIBS)

mHPXBgCorrectScripts: mHPXBgCorrectScripts.o
			gcc -o mHPXBgCorrectScripts mHPXBgCorrectScripts.o $(LIBS)

mHPXCoaddScripts: mHPXCoaddScripts.o
			gcc -o mHPXCoaddScripts mHPXCoaddScripts.o $(LIBS)

mHPXShrinkScripts: mHPXShrinkScripts.o
			gcc -o mHPXShrinkScripts mHPXShrinkScripts.o $(LIBS)

mHPXQuicklookScripts: mHPXQuicklookScripts.o
			gcc -o mHPXQuicklookScripts mHPXQuicklookScripts.o $(LIBS)

mHiPSTileScripts: mHiPSTileScripts.o
			gcc -o mHiPSTileScripts mHiPSTileScripts.o $(LIBS)

mHiPSPNGScripts: mHiPSPNGScripts.o
			gcc -o mHiPSPNGScripts mHiPSPNGScripts.o $(LIBS)

mHPXBlendScripts: mHPXBlendScripts.o
			gcc -o mHPXBlendScripts mHPXBlendScripts.o $(LIBS)

mHPXHistogramScripts: mHPXHistogramScripts.o
			gcc -o mHPXHistogramScripts mHPXHistogramScripts.o $(LIBS)

mHPX8BitScripts: mHPX8BitScripts.o
			gcc -o mHPX8BitScripts mHPX8BitScripts.o $(LIBS)

mHPXCombineHistScripts: mHPXCombineHistScripts.o
			gcc -o mHPXCombineHistScripts mHPXCombineHistScripts.o $(LIBS)

mHPXPlatePNGScripts: mHPXPlatePNGScripts.o
			gcc -o mHPXPlatePNGScripts mHPXPlatePNGScripts.o $(LIBS)

mHPXColorQuicklookScripts: mHPXColorQuicklookScripts.o
			gcc -o mHPXColorQuicklookScripts mHPXColorQuicklookScripts.o $(LIBS)

mHPXFlattenScripts: mHPXFlattenScripts.o
			gcc -o mHPXFlattenScripts mHPXFlattenScripts.o $(LIBS)

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
