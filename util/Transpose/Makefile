# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        30Jul14  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mTranspose:	mTranspose.o
		$(CC) -o mTranspose mTranspose.o \
		../../Montage/checkHdr.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mTranspose ../../bin

clean:
		rm -f mTranspose *.o
