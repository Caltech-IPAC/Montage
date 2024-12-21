CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lwcs -lcfitsio -lcoord -lm

mTileImage: mTileImage.o montageTileImage.o \
  ../Subimage/montageSubimage.o \
  ../util/checkHdr.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mTileImage ../../bin

clean:
		rm -f mTileImage *.o
