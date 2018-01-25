CC      =		gcc
CFLAGS  =		-c -g -fPIC -I. -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBNAME =		libwww.a

SRCS    =		www.c

OBJS    =		${SRCS:.c=.o}


libwww.a:	${OBJS}
			rm -f ${LIBNAME}
			ar q ${LIBNAME} ${OBJS}

${OBJS}:		www.h

install:
			cp libwww.a ../..
			cp www.h ../../include

clean:
			rm -f libwww.a *.o *.a *.i a.out core
