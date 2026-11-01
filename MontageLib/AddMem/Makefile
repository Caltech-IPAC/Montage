CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lmtbl -lwcs -lcfitsio -lm

mAddMem: mAddMem.o montageAddMem.o \
  ../util/debugCheck.o ../util/filePath.o ../util/checkFile.o \
  ../util/checkHdr.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mAddMem ../../bin

clean:
		rm -f mAddMem *.o
