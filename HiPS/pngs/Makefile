.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CPPFLAGS +=	-I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS   =	-L../../lib -lmtbl -lwcs -lsvc -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all:		mHPXPlateHist mHPXPlateComboHist mHPXPlatePNG mHiPSPNGs

mHPXPlateHist:	mHPXPlateHist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXPlateComboHist:	mHPXPlateComboHist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHPXPlatePNG:	mHPXPlatePNG.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHiPSPNGs:	mHiPSPNGs.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mHPXPlateHist mHPXPlateComboHist mHPXPlatePNG mHiPSPNGs ../../bin

clean:
		rm -f mHPXPlateHist mHPXPlateComboHist mHPXPlatePNG mHiPSPNGs *.o
