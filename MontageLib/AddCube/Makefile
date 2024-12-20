.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mAddCube: mAddCube.o montageAddCube.o
	$(CC)  $(LDFLAGS)-o mAddCube mAddCube.o montageAddCube.o \
	../util/filePath.o ../util/debugCheck.o ../util/checkHdr.o ../util/checkWCS.o \
	$(LIBS)

install:
		cp mAddCube ../../bin

clean:
		rm -f mAddCube *.o
