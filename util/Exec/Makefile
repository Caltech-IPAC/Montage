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

all:		mExec

mExec:		mExec.o
		$(CC) -o mExec mExec.o \
		../../Montage/debugCheck.o   \
		../../Montage/filePath.o   \
		$(LIBS)

install:
		cp mExec ../../bin

clean:
		rm -f mExec *.o
