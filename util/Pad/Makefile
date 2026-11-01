# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        02Apr11  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mPad:		mPad.o
		$(CC) -o mPad mPad.o \
		../../Montage/checkHdr.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mPad ../../bin

clean:
		rm -f mPad *.o
