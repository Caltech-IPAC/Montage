.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lcmd -lsvc -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mTest:		mTest.o
		$(CC) -o mTest mTest.o $(LIBS)

install:
		cp mTest ../../bin

clean:
		rm -f mTest *.o
