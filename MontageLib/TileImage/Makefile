.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mTileImage:	mTileImage.o montageTileImage.o
		$(CC) -o mTileImage mTileImage.o montageTileImage.o \
		../util/checkHdr.o \
		../util/checkWCS.o \
		../Subimage/montageSubimage.o \
		$(LIBS)

install:
		cp mTileImage ../../bin

clean:
		rm -f mTileImage *.o
