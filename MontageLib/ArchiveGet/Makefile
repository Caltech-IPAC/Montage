.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -fPIC
LIBS   =	 -L../../lib -lbz2

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mArchiveGet:	mArchiveGet.o montageArchiveGet.o
		$(CC) -o mArchiveGet mArchiveGet.o montageArchiveGet.o $(LIBS)

install:
		cp mArchiveGet ../../bin

clean:
		rm -f mArchiveGet *.o
