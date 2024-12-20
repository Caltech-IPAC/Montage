.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all:		filePath.o debugCheck.o checkFile.o checkHdr.o checkWCS.o version.o

clean:
			rm -f *.o
