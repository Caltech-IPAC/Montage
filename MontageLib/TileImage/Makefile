.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mTileImage: mTileImage.o montageTileImage.o
	$(CC) $(LDFLAGS) -o mTileImage mTileImage.o montageTileImage.o \
	../util/checkHdr.o ../util/checkWCS.o \
	../Subimage/montageSubimage.o \
	$(LIBS)

install:
		cp mTileImage ../../bin

clean:
		rm -f mTileImage *.o
