CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lcfitsio -lm

all:		mHistogram mCombineHist

mHistogram: mHistogram.o montageHistogram.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mCombineHist: mCombineHist.o montageCombineHist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mHistogram mCombineHist ../../bin

clean:
		rm -f mHistogram mCombineHist *.o
