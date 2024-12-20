# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        03Jan11  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

DATA_DIR = $(shell cd ../../data; pwd)

CC     =        gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -I../../lib/freetype/include \
		-I../../lib/freetype/include/freetype2 -I../../Montage \
                -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
                -DFONT_DIR='"$(DATA_DIR)/fonts/"'
CFLAGS   +=	-std=c99 -fPIC
LIBS   =        -L../../lib -lwcs -lcoord -lcfitsio -ljpeg -llodepng -lmtbl -ljson -lcmd \
                -L../../lib/freetype/lib -lfreetype -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mViewer:	mViewer.o montageViewer.o mViewer_graphics.o mViewer_grid.o mViewer_boundingbox.o
		$(CC) -o mViewer mViewer.o montageViewer.o mViewer_graphics.o mViewer_grid.o mViewer_boundingbox.o \
		../util/checkHdr.o   \
		../util/checkWCS.o   \
		$(LIBS)

install:
		cp mViewer ../../bin

clean:
		rm -f mViewer *.o
