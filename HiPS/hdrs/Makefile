.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc
CPPFLAGS +=	-I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   = -L../../lib -lwcs -lboundaries -lmtbl -lsvc -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c


all:      mHPXHdr mHiPSHdr 

mHPXHdr: mHPXHdr.o
			$(CC) $(LDFLAGS) -o mHPXHdr mHPXHdr.c $(LIBS)

mHiPSHdr: mHiPSHdr.o
			$(CC) $(LDFLAGS) -o mHiPSHdr mHiPSHdr.o $(LIBS)

install:
			cp mHPXHdr mHiPSHdr ../../bin

clean:
			rm -f mHPXHdr mHiPSHdr *.o
