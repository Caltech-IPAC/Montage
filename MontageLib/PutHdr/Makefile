.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mPutHdr: mPutHdr.o montagePutHdr.o
	$(CC) $(LDFLAGS) -o mPutHdr mPutHdr.o montagePutHdr.o \
	../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o \
	$(LIBS)

install:
		cp mPutHdr ../../bin

clean:
		rm -f mPutHdr *.o
