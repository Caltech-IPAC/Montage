# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        02Apr11  Original SunOS Makefile

CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS      =	-L../../lib -lwcs -lcfitsio -lm

mPad: mPad.o ../../Montage/checkHdr.o ../../Montage/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mPad ../../bin

clean:
		rm -f mPad *.o
