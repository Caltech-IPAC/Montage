CC      =		gcc
CPPFLAGS  +=		-I. -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS    +=		-fPIC

SRCS    =		boundaries.c

OBJS    =		${SRCS:.c=.o}


libboundaries.a:	${OBJS}
			ar rv $@ $^

${OBJS}:		boundaries.h

install:
			cp libboundaries.a ../..
			cp boundaries.h ../../include

clean:
			rm -f libboundaries.a *.o *.a *.i a.out core
