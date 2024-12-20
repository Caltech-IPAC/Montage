# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        03Jan11  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

DATA_DIR = $(shell cd ../../data; pwd)

CC     =        gcc
CPPFLAGS +=	-I. -I../../lib/include -I../../lib/freetype/include \
		-I../../lib/freetype/include/freetype2 -I../../Montage \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64  \
                -DFONT_DIR='"$(DATA_DIR)/fonts/"'
CFLAGS   +=	-std=c99
LIBS   =	-L../../lib -lwcs -lcoord -lcfitsio -ljpeg -llodepng -lmtbl -lcmd \
		-L../../lib/freetype/lib -lfreetype -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all: mViewer mHistogram

mViewer: mViewer.o graphics.o grid.o \
  ../../Montage/checkHdr.o ../../Montage/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHistogram: mHistogram.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mViewer mHistogram ../../bin

clean:
		rm -f mViewer mHistogram *.o
