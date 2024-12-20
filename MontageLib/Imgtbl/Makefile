.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lcoord -lmtbl -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mImgtbl: mImgtbl.o montageImgtbl.o
	$(CC) $(LDFLAGS) -o mImgtbl mImgtbl.o montageImgtbl.o \
	../util/checkWCS.o \
	$(LIBS)

install:
		cp mImgtbl ../../bin

clean:
		rm -f mImgtbl *.o
