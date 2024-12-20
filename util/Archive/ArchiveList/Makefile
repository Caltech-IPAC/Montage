# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original Darwin Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	cc
CPPFLAGS +=	-I. -I../../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   =	-L../../../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		-lboundaries -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mArchiveList:	mArchiveList.o
		$(CC) $(LDFLAGS) -o mArchiveList mArchiveList.o $(LIBS)

install:
		cp mArchiveList ../../../bin

clean:
		rm -f mArchiveList *.o
