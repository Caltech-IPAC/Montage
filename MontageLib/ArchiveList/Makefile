# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original Darwin Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	cc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -fPIC $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lcoord  -lboundaries -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mArchiveList:	mArchiveList.o montageArchiveList.o
		$(CC) -o mArchiveList mArchiveList.o montageArchiveList.o $(LIBS)

install:
		cp mArchiveList ../../bin

clean:
		rm -f mArchiveList *.o
