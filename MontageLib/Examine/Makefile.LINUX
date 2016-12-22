.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -std=c99 -fPIC -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
CFLAGS =	-g -I. -I.. -I../../lib/include -Wall
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lnsl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mExamine:	mExamine.o montageExamine.o
		$(CC) -o mExamine mExamine.o montageExamine.o \
		../util/checkHdr.o   \
		../util/checkWCS.o   \
		$(LIBS)

install:
		cp mExamine ../../bin

clean:
		rm -f mExamine *.o
