CC = gcc
CPPFLAGS +=	-I . -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-fPIC

libmtbl.a:	mtbl.o
		ar rv libmtbl.a mtbl.o

install:
		cp libmtbl.a ../..
		cp mtbl.h ../../include

clean:		
		rm -f libmtbl.a *.o *.a core
