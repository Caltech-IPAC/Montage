CC =		 gcc -g -I. -fPIC -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

# WORKAROUND (issue #96): svc.h declares svc_sighandler() with `()`, which
# means `(void)` under the GCC 15 C23 default, so svclib.c's signal() calls
# and its definition fail. Remove once svc has real prototypes.
CFLAGS +=	-std=gnu17

libsvc.a:	svclib.o structlib.o
		ar rv libsvc.a svclib.o structlib.o

svclib.o:	svclib.c svc.h
structlib.o:	structlib.c svc.h

install:
		cp libsvc.a ../..
		cp svc.h ../../include

clean:
		rm -f libsvc.a *.o *.a
