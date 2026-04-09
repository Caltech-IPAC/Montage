# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        25Sep04  Original SunOS Makefile

CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../lib -lwcs -lcoord -lcfitsio -ljpeg -lm

all:		mPix2Coord mFixHdr

mPix2Coord: mPix2Coord.o ../../Montage/checkHdr.o ../../Montage/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mFixHdr: mFixHdr.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mPix2Coord mFixHdr ../../bin

clean:
		rm -f mPix2Coord mFixHdr *.o
