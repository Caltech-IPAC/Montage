CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lwcs -lmtbl -lpixbounds -lcfitsio -lm

mFitExec: mFitExec.o montageFitExec.o \
  ../Fitplane/montageFitplane.o \
  ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o ../util/checkFile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mFitExec ../../bin

clean:
		rm -f mFitExec *.o
