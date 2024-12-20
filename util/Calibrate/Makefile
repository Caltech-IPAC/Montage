# Filename: Makefile.LINUX

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        15Nov15  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS   =	-L../../lib -lmtbl -lsvc -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all:  mCalibrate mCalExec

mCalibrate:		mCalibrate.o
		$(CC) $(LDFLAGS) -o mCalibrate mCalibrate.o $(LIBS)

mCalExec:		mCalExec.o
		$(CC) $(LDFLAGS) -o mCalExec mCalExec.o ../../Montage/filePath.o $(LIBS)

install:
		cp mCalibrate mCalExec ../../bin

clean:
		rm -f mCalibrate mCalExec *.o
