# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.6      Mats Rynge       09Jun10  Added mDAGGalacticPlane
# 1.5      John Good        29Mar06  Added mDAG (developed elsewhere 
#       		             originally)
# 1.4      John Good        24Nov04  Added mGridExec
# 1.3      John Good        04Nov04  Added mPresentation
# 1.2      John Good        08Oct04  Added mConcatFit mDiffFit
# 1.1      John Good        02Jun04  Added 64-bit addressing flag
# 1.0      John Good        16Oct03  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS =	-g -O2 -I. -I../../Montage -I../../lib/include
LIBS   =	-L../../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		-lboundaries -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		mConcatFit mDAG mDAGFiles mDAGGalacticPlane mDAGTbls \
		mDiffFit mGridExec mNotify mPresentation

mConcatFit:	mConcatFit.o
		$(CC) -o mConcatFit mConcatFit.o \
		../../Montage/checkFile.o $(LIBS)

mDAG:		mDAG.o hashtable.o
		$(CC) -o mDAG mDAG.o hashtable.o $(LIBS)

mDAGFiles:	mDAGFiles.o
		$(CC) -o mDAGFiles mDAGFiles.o $(LIBS)

mDAGGalacticPlane:		mDAGGalacticPlane.o hashtable.o
		$(CC) -o mDAGGalacticPlane mDAGGalacticPlane.o hashtable.o $(LIBS)

mDAGTbls:	mDAGTbls.o
		$(CC) -o mDAGTbls mDAGTbls.o  \
		../../Montage/filePath.o  \
		../../Montage/checkHdr.o  \
		../../Montage/checkWCS.o $(LIBS)

mDiffFit:	mDiffFit.o
		$(CC) -o mDiffFit mDiffFit.o $(LIBS)

mGridExec:	mGridExec.o
		$(CC) -o mGridExec mGridExec.o $(LIBS)

mNotify:	mNotify.o
		$(CC) -o mNotify mNotify.o $(LIBS)

mPresentation:	mPresentation.o
		$(CC) -o mPresentation mPresentation.o $(LIBS)

install:
		cp mConcatFit mDAG mDAGFiles mDAGGalacticPlane mDAGTbls \
		mDiffFit mGridExec mNotify mPresentation ../../bin

clean:
		rm -f mConcatFit mDAG mDAGFiles mDAGGalacticPlane mDAGTbls \
		mDiffFit mGridExec mNotify mPresentation *.o
