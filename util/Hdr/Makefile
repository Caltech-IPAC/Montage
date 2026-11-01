# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        21Jul06  Original Darwin Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	cc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I../../lib/include $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lcoord -lmtbl -lsvc \
		-lboundaries -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mHdr:	mHdr.o
		$(CC) -o mHdr mHdr.o $(LIBS)

install:
		cp mHdr ../../bin

clean:
		rm -f mHdr *.o
