.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lmtbl -lwcs -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mAddMem:	mAddMem.o montageAddMem.o
			$(CC) -o mAddMem mAddMem.o montageAddMem.o ../util/debugCheck.o ../util/filePath.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o $(LIBS)

install:
		cp mAddMem ../../bin

clean:
		rm -f mAddMem *.o
