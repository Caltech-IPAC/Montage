# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original Darwin Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	cc
CPPFLAGS +=	-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I. -I.. -I../../lib/include
CFLAGS   +=	-fPIC
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord  -lboundaries -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mArchiveList:	mArchiveList.o montageArchiveList.o
		$(CC) -o mArchiveList mArchiveList.o montageArchiveList.o $(LIBS)

install:
		cp mArchiveList ../../bin

clean:
		rm -f mArchiveList *.o
