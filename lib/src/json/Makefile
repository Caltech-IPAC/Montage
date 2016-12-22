CC =		 gcc -g -I. -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

libjson.a:	jsonlib.o
		ar rv libjson.a jsonlib.o

jsonlib.o:	jsonlib.c json.h

install:
		cp libjson.a ../..
		cp json.h ../../include

clean:
		rm -f libjson.a *.o *.a
