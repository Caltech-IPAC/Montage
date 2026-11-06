CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lwcs -lmtbl -lpixbounds -lcfitsio -lm

mDiffFit: mDiffFit.o montageDiffFit.o \
  ../Diff/montageDiff.o ../Fitplane/montageFitplane.o \
  ../util/checkHdr.o ../util/checkWCS.o ../util/filePath.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mDiffFit ../../bin

clean:
		rm -f mDiffFit *.o
