# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        20Feb07  Original Darwin Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I./rtree -I../../lib/include -I../../Montage
LIBS   =	-Lrtree -lrtree -L../../lib -lcmd -lwcs -lcoord -lmtbl -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mSearch:	mSearch.o
		(cd rtree; make)
		$(CC) -o mSearch mSearch.o \
		../../Montage/debugCheck.o   \
		../../Montage/checkFile.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mSearch ../../bin

clean:
		(cd rtree; make clean)
		rm -f mSearch *.o
