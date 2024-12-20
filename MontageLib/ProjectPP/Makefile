.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -ltwoplane -lcoord -lwcs -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mProjectPP: mProjectPP.o montageProjectPP.o
	$(CC) $(LDFLAGS) -o mProjectPP mProjectPP.o montageProjectPP.o \
	../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o \
	$(LIBS)

install:
		cp mProjectPP ../../bin

clean:
		rm -f mProjectPP *.o
