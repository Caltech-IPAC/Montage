.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lcmd -lmtbl -ljson -lcoord -lwcs -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mMakeImg: mMakeImg.o montageMakeImg.o
	$(CC) $(LDFLAGS) -o mMakeImg mMakeImg.o montageMakeImg.o \
	$(LIBS)

install:
		cp mMakeImg ../../bin

clean:
		rm -f mMakeImg *.o
