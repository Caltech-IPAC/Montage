CFITSIO_LIBS ?= -lcfitsio
HPXcvt:		HPXcvt.o
				gcc -std=gnu99 -o HPXcvt HPXcvt.o -L../lib $(CFITSIO_LIBS) -lm

HPXcvt.o:	HPXcvt.c
				gcc -std=gnu99 -g -O2 -I../lib/include $(CFITSIO_CFLAGS) -c -o HPXcvt.o HPXcvt.c

install:
				cp HPXcvt ../bin

clean:
				rm -f HPXcvt *.o
