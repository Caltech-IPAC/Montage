# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        21Jul06  Original Darwin Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	cc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I../../lib/include
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		-lboundaries -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mHdr:	mHdr.o
		$(CC) -o mHdr mHdr.o $(LIBS)

install:
		cp mHdr ../../bin

clean:
		rm -f mHdr *.o
