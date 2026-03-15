CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lwww -lwcs -lcfitsio -lm

mGetHdr: mGetHdr.o montageGetHdr.o \
  ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mGetHdr ../../bin

clean:
		rm -f mGetHdr *.o
