# Filename: Makefile.LINUX

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        15Nov15  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
CFLAGS =	-g -I. -I../../lib/include -I../../Montage
LIBS   =	-L../../lib -lmtbl -lsvc -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mCalibrate:		mCalibrate.o
		$(CC) -o mCalibrate mCalibrate.o $(LIBS)

install:
		cp mCalibrate ../../bin

clean:
		rm -f mCalibrate *.o
