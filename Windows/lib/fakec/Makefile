libc.a:     fakec.c
				$(CC) -I. -c fakec.c
				ar rv libc.a fakec.o

install:
				cp libc.a /usr/src/mxe/usr/x86_64-w64-mingw32.static/lib

clean:
				rm -f libc.a *.o *.a core
