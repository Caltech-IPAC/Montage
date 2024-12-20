.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lcoord -lwcs -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mSubCube: mSubCube.o montageSubCube.o
	$(CC) $(LDFLAGS) -o mSubCube mSubCube.o montageSubCube.o \
	../util/debugCheck.o ../util/checkFile.o ../util/checkHdr.o ../util/checkWCS.o \
	$(LIBS)

install:
		cp mSubCube ../../bin

clean:
		rm -f mSubCube *.o
