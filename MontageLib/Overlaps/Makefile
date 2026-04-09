CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lcoord -lwcs -lmtbl -lm

mOverlaps: mOverlaps.o montageOverlaps.o \
  ../util/debugCheck.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mOverlaps ../../bin

clean:
		rm -f mOverlaps *.o
