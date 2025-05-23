CPPFLAGS  +=	-I. -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS    +=	-fPIC

SRCS    =		www.c

OBJS    =		${SRCS:.c=.o}


libwww.a:	${OBJS}
		ar rv $@ $^

${OBJS}:		www.h

install:
			cp libwww.a ../..
			cp www.h ../../include

clean:
			rm -f libwww.a *.o *.a *.i a.out core
