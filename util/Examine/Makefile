# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        13Feb08  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lcoord -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mExamine:	mExamine.o
		$(CC) -o mExamine mExamine.o \
		../../Montage/checkHdr.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mExamine ../../bin

clean:
		rm -f mExamine *.o
