.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		filePath.o debugCheck.o checkFile.o checkHdr.o checkWCS.o version.o

clean:
			rm -f *.o
