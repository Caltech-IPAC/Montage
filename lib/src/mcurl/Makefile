CC      =		gcc
CFLAGS  =		-c -g -I. -I../../include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBNAME =		libmcurl.a

SRCS    =		mcurl.c

OBJS    =		${SRCS:.c=.o}


libmcurl.a:		${OBJS}
			rm -f ${LIBNAME}
			ar q ${LIBNAME} ${OBJS}

${OBJS}:		mcurl.h

install:
			cp libmcurl.a ../..
			cp mcurl.h ../../include

clean:
			rm -f libmcurl.a *.o *.a *.i a.out core
