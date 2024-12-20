.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mAdd:		mAdd.o montageAdd.o
		$(CC) -o mAdd mAdd.o montageAdd.o ../util/filePath.o ../util/debugCheck.o ../util/checkHdr.o \
		../util/checkWCS.o $(LIBS)

install:
		cp mAdd ../../bin

clean:
		rm -f mAdd *.o
