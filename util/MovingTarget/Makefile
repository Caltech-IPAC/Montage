# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        10Apr15  Original LINUX Makefile (from mSearch)

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I./rtree -I../../lib/include -I../../Montage $(CFITSIO_CFLAGS)
LIBS   =	-Lrtree -lrtree -L../../lib -lcmd -lwcs -lcoord -lmtbl $(CFITSIO_LIBS) -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mMovingTarget:	mMovingTarget.o
		(cd rtree; make)
		$(CC) -o mMovingTarget mMovingTarget.o \
		../../Montage/debugCheck.o   \
		../../Montage/checkFile.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mMovingTarget ../../bin

clean:
		(cd rtree; make clean)
		rm -f mMovingTarget *.o
