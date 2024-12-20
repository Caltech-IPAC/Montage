.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mTANHdr:		mTANHdr.o montageTANHdr.o
		$(CC) -o mTANHdr mTANHdr.o montageTANHdr.o ../util/checkHdr.o ../util/checkWCS.o ../util/checkFile.o $(LIBS)

install:
		cp mTANHdr ../../bin

clean:
		rm -f mTANHdr *.o
