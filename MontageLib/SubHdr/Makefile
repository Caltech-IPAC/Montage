CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lcoord -lwcs -lcfitsio -lm

mSubHdr: mSubHdr.o montageSubHdr.o \
  ../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mSubHdr ../../bin

clean:
		rm -f mSubHdr *.o
