# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original Darwin Makefile

CPPFLAGS +=	-I. -I../../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		-lboundaries -lm

mArchiveList: mArchiveList.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mArchiveList ../../../bin

clean:
		rm -f mArchiveList *.o
