CPPFLAGS +=	-I. -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-fPIC

SRCS    =               pixbounds.c

OBJS    =               ${SRCS:.c=.o}

libpixbounds.a:		pixbounds.o
			ar rv $@ $^

install:
			cp libpixbounds.a ../..
			cp pixbounds.h ../../include

clean:
			rm -f bndtest libpixbounds.a *.o *.a *.i a.out core

test:			bndtest.c
			$(CC) -o bndtest bndtest.c -I. -L. -lpixbounds -lm

runtest:
			bndtest < test.txt
