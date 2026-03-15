# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original Darwin Makefile

CPPFLAGS +=	-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I. -I.. -I../../lib/include
CFLAGS   +=	-fPIC
LIBS      =	-L../../lib -lwcs -lcfitsio -lcoord  -lboundaries -lm

mArchiveList: mArchiveList.o montageArchiveList.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mArchiveList ../../bin

clean:
		rm -f mArchiveList *.o
