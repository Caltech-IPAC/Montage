.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mAddCube:		mAddCube.o montageAddCube.o
					$(CC) -o mAddCube mAddCube.o montageAddCube.o ../util/filePath.o ../util/debugCheck.o ../util/checkHdr.o \
					../util/checkWCS.o $(LIBS)

install:
		cp mAddCube ../../bin

clean:
		rm -f mAddCube *.o
