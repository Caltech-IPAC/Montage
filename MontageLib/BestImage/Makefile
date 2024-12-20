# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------
# 1.0      John Good        14Feb05  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mBestImage:	mBestImage.o montageBestImage.o \
		../util/checkWCS.o \
		../util/filePath.o \
		../util/debugCheck.o
		$(CC) -o mBestImage \
		mBestImage.o montageBestImage.o \
		../util/checkWCS.o \
		../util/filePath.o \
		../util/debugCheck.o $(LIBS)

install:
		cp mBestImage ../../bin

clean:
		rm -f mBestImage *.o
