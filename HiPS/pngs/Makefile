.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CFLAGS =	-g -I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
LIBS   =	-L../../lib -lwcs -lsvc -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mHPXPlateHist mHPXPlateComboHist mHPXPlatePNG mHiPSPNGs

mHPXPlateHist:	mHPXPlateHist.o
		$(CC) -o mHPXPlateHist mHPXPlateHist.o $(LIBS)

mHPXPlateComboHist:	mHPXPlateComboHist.o
		$(CC) -o mHPXPlateComboHist mHPXPlateComboHist.o $(LIBS)

mHPXPlatePNG:	mHPXPlatePNG.o
		$(CC) -o mHPXPlatePNG mHPXPlatePNG.o $(LIBS)

mHiPSPNGs:	mHiPSPNGs.o
		$(CC) -o mHiPSPNGs mHiPSPNGs.o $(LIBS)

install:
		cp mHPXPlateHist mHPXPlateComboHist mHPXPlatePNG mHiPSPNGs ../../bin

clean:
		rm - mHPXPlateHist mHPXPlateComboHist mHPXPlatePNG mHiPSPNGs *.o
