.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include
CFLAGS   +=	-fPIC
LIBS   =	 -L../../lib -lbz2

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mArchiveGet:	mArchiveGet.o montageArchiveGet.o
		$(CC) -o mArchiveGet mArchiveGet.o montageArchiveGet.o $(LIBS)

install:
		cp mArchiveGet ../../bin

clean:
		rm -f mArchiveGet *.o
