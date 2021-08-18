.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g -O2 -I. -I../../lib/include
LIBS   = -L../../lib -lwcs -lboundaries -lmtbl -lsvc -lm

.c.o:
			$(CC) $(CFLAGS)  -c  $*.c


all:      mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgScripts mHPXCoaddScripts \
          mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts 

mHPXMosaicScripts: mHPXMosaicScripts.o
			gcc -o mHPXMosaicScripts mHPXMosaicScripts.o $(LIBS)

mHPXProjectScripts: mHPXProjectScripts.o
			gcc -o mHPXProjectScripts mHPXProjectScripts.o $(LIBS)

mHPXDiffScripts: mHPXDiffScripts.o
			gcc -o mHPXDiffScripts mHPXDiffScripts.o $(LIBS)

mHPXBgScripts: mHPXBgScripts.o
			gcc -o mHPXBgScripts mHPXBgScripts.o $(LIBS)

mHPXCoaddScripts: mHPXCoaddScripts.o
			gcc -o mHPXCoaddScripts mHPXCoaddScripts.o $(LIBS)

mHPXShrinkScripts: mHPXShrinkScripts.o
			gcc -o mHPXShrinkScripts mHPXShrinkScripts.o $(LIBS)

mHiPSTileScripts: mHiPSTileScripts.o
			gcc -o mHiPSTileScripts mHiPSTileScripts.o $(LIBS)

mHiPSPNGScripts: mHiPSPNGScripts.o
			gcc -o mHiPSPNGScripts mHiPSPNGScripts.o $(LIBS)

install:
			cp mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgScripts mHPXCoaddScripts \
			   mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts ../../bin

clean:
			rm -f mHPXMosaicScripts mHPXProjectScripts mHPXDiffScripts mHPXBgScripts mHPXCoaddScripts \
			   mHPXShrinkScripts mHiPSTileScripts mHiPSPNGScripts *.o
