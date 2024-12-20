CPPFLAGS +=	-I. -I.. -I../../lib/include
CFLAGS   +=	-fPIC
LIBS      =	 -L../../lib -lbz2

mArchiveGet: mArchiveGet.o montageArchiveGet.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mArchiveGet ../../bin

clean:
		rm -f mArchiveGet *.o
