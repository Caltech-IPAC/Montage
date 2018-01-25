CC      =   gcc
CFLAGS  =   -c -ansi -Wall -fPIC -g -I. -I../../include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

liblodepng.a:	lodepng.o 
				ar rv liblodepng.a lodepng.o 

install:
				cp liblodepng.a ../..
				cp lodepng.h ../../include

clean:
				rm -f *.o *.a core
