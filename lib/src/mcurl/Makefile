CPPFLAGS +=		-I. -I../../include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

SRCS    =		mcurl.c

OBJS    =		${SRCS:.c=.o}


libmcurl.a:		${OBJS}
			ar rv $@ $^

${OBJS}:		mcurl.h

install:
			cp libmcurl.a ../..
			cp mcurl.h ../../include

clean:
			rm -f libmcurl.a *.o *.a *.i a.out core
