.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lboundaries -lmtbl -lcoord -lwcs -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mCoverageCheck:	mCoverageCheck.o montageCoverageCheck.o
	$(CC)  $(LDFLAGS) -o mCoverageCheck mCoverageCheck.o montageCoverageCheck.o \
	../util/debugCheck.o ../util/checkFile.o \
	../util/checkHdr.o ../util/checkWCS.o \
	$(LIBS)

install:
		cp mCoverageCheck ../../bin

clean:
		rm -f mCoverageCheck *.o
