# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        25Sep04  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage
LIBS   =	-L../../lib -lwcs -lcoord -lcfitsio -ljpeg -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mPix2Coord mFixHdr

mPix2Coord:	mPix2Coord.o
		$(CC) -o mPix2Coord mPix2Coord.o \
		../../Montage/checkHdr.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

mFixHdr:	mFixHdr.o
		$(CC) -o mFixHdr mFixHdr.o

install:
		cp mPix2Coord mFixHdr ../../bin

clean:
		rm -f mPix2Coord mFixHdr *.o
