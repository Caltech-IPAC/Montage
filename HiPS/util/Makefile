.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g -I. -I../../lib/include
LIBS   = -L../../lib -lcfitsio -lwcs -lboundaries -lmtbl -lsvc -lm

.c.o:
			$(CC) $(CFLAGS)  -c  $*.c


all:    					mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine

mHPXPlateList: 		mHPXPlateList.o
							gcc -o mHPXPlateList mHPXPlateList.o $(LIBS)

mHiPSSetup:				mHiPSSetup.o
							gcc -o mHiPSSetup mHiPSSetup.o $(LIBS)

mHiPSID:					mHiPSID.o
							gcc -o mHiPSID mHiPSID.o $(LIBS)

mHiPSAllSky: 			mHiPSAllSky.o
							gcc -o mHiPSAllSky mHiPSAllSky.o $(LIBS)

mHPXPlateCombine: 	mHPXPlateCombine.o
							gcc -o mHPXPlateCombine mHPXPlateCombine.o $(LIBS)

install:
							cp mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine ../../bin

clean:
							rm -f mHPXPlateList mHiPSSetup mHiPSID mHiPSAllSky mHPXPlateCombine *.o
