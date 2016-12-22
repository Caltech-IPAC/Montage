.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lmtbl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mBgModel:	mBgModel.o montageBgModel.o
				$(CC) -o mBgModel mBgModel.o montageBgModel.o ../util/debugCheck.o $(LIBS)

install:
		cp mBgModel ../../bin

clean:
		rm -f mBgModel *.o
