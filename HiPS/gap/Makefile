CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lpixbounds -lwcs -lmtbl -lcfitsio -lm

all:		mHPXGapScripts mHPXGapDiff mHPXGapCombine

mHPXGapScripts: mHPXGapScripts.o ../../MontageLib/util/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXGapDiff: mHPXGapDiff.o montageHPXGapDiff.o ../../MontageLib/util/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXGapCombine: mHPXGapCombine.o ../../MontageLib/util/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mHPXGapScripts mHPXGapDiff mHPXGapCombine ../../bin

clean:
		rm -f mHPXGapScripts mHPXGapDiff mHPXGapCombine *.o
