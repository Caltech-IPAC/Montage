.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lnsl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mAdd:		mAdd.o montageAdd.o
		$(CC) -o mAdd mAdd.o montageAdd.o ../util/filePath.o ../util/debugCheck.o ../util/checkHdr.o \
		../util/checkWCS.o $(LIBS)

install:
		cp mAdd ../../bin

clean:
		rm -f mAdd *.o
