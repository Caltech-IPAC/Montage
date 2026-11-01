# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        11May05  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lcoord -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mRotate:	mRotate.o
		$(CC) -o mRotate mRotate.o \
		../../Montage/checkHdr.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mRotate ../../bin

clean:
		rm -f mRotate *.o
