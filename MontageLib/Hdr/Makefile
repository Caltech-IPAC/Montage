.SUFFIXES:
.SUFFIXES: .c .o

CC     =	cc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lmtbl \
		-lboundaries -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mHdr: mHdr.o montageHdr.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mHdr ../../bin

clean:
		rm -f mHdr *.o
