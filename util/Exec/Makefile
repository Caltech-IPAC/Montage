# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        04Oct05  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage
LIBS   =	-L../../lib -lsvc -lmtbl -lcmd -lwcs -lcfitsio -lcoord -lboundaries -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mExec mImgBounds

mExec:		mExec.o
		$(CC) -o mExec mExec.o \
		../../Montage/debugCheck.o   \
		../../Montage/filePath.o   \
		$(LIBS)

mImgBounds:	mImgBounds.o
		$(CC) -o mImgBounds mImgBounds.o \
		../../Montage/debugCheck.o   \
		../../Montage/filePath.o   \
		$(LIBS)

install:
		cp mExec mImgBounds ../../bin

clean:
		rm -f mExec mImgBounds *.o
