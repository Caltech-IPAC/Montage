.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC
LIBS   =	-L.. -lmontage -L../../lib -lwww -lpixbounds -ltwoplane -lboundaries -lcoord -lmtbl -lwcs -lcfitsio -lnsl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all: runall

runall:			runall.o
				$(CC) -o runall runall.o $(LIBS)

clean:
				rm -f runall projtest *.o
