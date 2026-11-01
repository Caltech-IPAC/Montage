CPPFLAGS  +=		-I. -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS    +=		-fPIC

SRCS    =		cmd.c

OBJS    =		${SRCS:.c=.o}


libcmd.a:		${OBJS}
			ar rv $@ $^

${OBJS}:		cmd.h

install:
			cp libcmd.a ../..
			cp cmd.h ../../include

clean:
			rm -f libcmd.a *.o *.a *.i a.out core
