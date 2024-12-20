.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CPPFLAGS +=	-I. -I../../MontageLib -I../../lib/include \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS   =	-L../../lib -lwcs -lcfitsio -lpixbounds -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all:		mHiPSTiles

mHiPSTiles:	mHiPSTiles.o
		$(CC) -o mHiPSTiles mHiPSTiles.o $(LIBS)

install:
		cp mHiPSTiles ../../bin
clean:
		rm -f mHiPSTiles *.o
