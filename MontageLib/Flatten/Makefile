.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lpixbounds -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mFlatten:	mFlatten.o montageFlatten.o
		$(CC) -o mFlatten mFlatten.o montageFlatten.o \
                      ../Fitplane/montageFitplane.o \
                      ../Background/montageBackground.o \
                      ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o $(LIBS)

install:
		cp mFlatten ../../bin

clean:
		rm -f mFlatten *.o
