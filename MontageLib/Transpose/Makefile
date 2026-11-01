# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        30Jul14  Original SunOS Makefile

CPPFLAGS +=	-I. -I.. -I../../lib/include  -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lwcs -lcfitsio -lcoord -lm

mTranspose: mTranspose.o montageTranspose.o \
  ../util/checkHdr.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mTranspose ../../bin

clean:
		rm -f mTranspose *.o
