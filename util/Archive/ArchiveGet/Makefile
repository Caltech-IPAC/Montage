# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        14Dec04  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CFITSIO_LIBS ?= -lcfitsio
CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I../../../Montage -I../../../lib/include $(CFITSIO_CFLAGS)
LIBS   =	-L../../../lib -lwcs $(CFITSIO_LIBS) -lmcurl -lcurl -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mArchiveGet:	mArchiveGet.o
		$(CC) -o mArchiveGet mArchiveGet.o  \
		../../../Montage/checkHdr.o         \
		../../../Montage/checkWCS.o $(LIBS)

install:
		cp mArchiveGet ../../../bin

clean:
		rm -f mArchiveGet *.o
