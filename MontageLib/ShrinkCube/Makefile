.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lwcs $(CFITSIO_LIBS) -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mShrinkCube:	mShrinkCube.o montageShrinkCube.o
					$(CC) -o mShrinkCube mShrinkCube.o montageShrinkCube.o ../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o $(LIBS)

install:
		cp mShrinkCube ../../bin

clean:
		rm -f mShrinkCube *.o
