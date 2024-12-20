.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all:		mHistogram mCombineHist

mHistogram:	mHistogram.o montageHistogram.o
		$(CC) -o mHistogram mHistogram.o montageHistogram.o $(LIBS)

mCombineHist:	mCombineHist.o montageCombineHist.o
		$(CC) -o mCombineHist mCombineHist.o montageCombineHist.o $(LIBS)

install:
		cp mHistogram mCombineHist ../../bin

clean:
		rm -f mHistogram mCombineHist *.o
