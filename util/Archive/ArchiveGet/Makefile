# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original SunOS Makefile

CPPFLAGS +=	-I../../../Montage -I../../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../../lib -lwcs -lcfitsio -lmcurl -lcurl -lm

mArchiveGet: mArchiveGet.o ../../../Montage/checkHdr.o ../../../Montage/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mArchiveGet ../../../bin

clean:
		rm -f mArchiveGet *.o
