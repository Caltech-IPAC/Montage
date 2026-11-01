# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        30Jul14  Original SunOS Makefile

CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../lib -lwcs -lcfitsio -lcoord -lm

mTranspose: mTranspose.o ../../Montage/checkHdr.o ../../Montage/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mTranspose ../../bin

clean:
		rm -f mTranspose *.o
