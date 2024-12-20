CC      =       gcc
CPPFLAGS +=	-I. -I../../include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-ansi -fPIC

libtwoplane.a:	two_plane.o distort.o initdistdata.o undistort.o redefine_pointing.o
		ar rv $@ $^

install:
		cp libtwoplane.a ../..
		cp two_plane.h distort.h ../../include

clean:
		rm -f libtwoplane.a *.o *.a core
