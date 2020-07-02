.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g -I. -I../../lib/include
LIBS   = -L../../lib -lwcs -lboundaries -lmtbl -lsvc -lm

.c.o:
			$(CC) $(CFLAGS)  -c  $*.c


all:      mHPXHdr mHiPSHdr 

mHPXHdr: mHPXHdr.o
			gcc -o mHPXHdr mHPXHdr.c $(LIBS)

mHiPSHdr: mHiPSHdr.o
			gcc -o mHiPSHdr mHiPSHdr.o $(LIBS)

install:
			cp mHPXHdr mHiPSHdr ../../bin

clean:
			rm -f mHPXHdr mHiPSHdr *.o
