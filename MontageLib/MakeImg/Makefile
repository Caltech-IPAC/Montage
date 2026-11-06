.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall $(CFITSIO_CFLAGS)
LIBS   =	-L../../lib -lcmd -lmtbl -ljson -lcoord -lwcs $(CFITSIO_LIBS) -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mMakeImg:	mMakeImg.o montageMakeImg.o
			$(CC) -o mMakeImg mMakeImg.o montageMakeImg.o $(LIBS)

install:
		cp mMakeImg ../../bin

clean:
		rm -f mMakeImg *.o
