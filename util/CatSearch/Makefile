# Filename: Makefile.LINUX

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        15Nov15  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
CFLAGS =	-g -O2 -I. -I../../lib/include -I../../Montage
LIBS   =	-L../../lib -lsvc -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mCatSearch:		mCatSearch.o
		$(CC) -o mCatSearch mCatSearch.o $(LIBS)

install:
		cp mCatSearch ../../bin

clean:
		rm -f mCatSearch *.o
