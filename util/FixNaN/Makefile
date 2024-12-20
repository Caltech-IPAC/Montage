# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        16Sep04  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   =	-L../../lib -lwcs -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mFixNaN:	mFixNaN.o
		$(CC) $(LDFLAGS) -o mFixNaN mFixNaN.o \
		../../Montage/checkHdr.o   \
		../../Montage/checkWCS.o   \
		$(LIBS)

install:
		cp mFixNaN ../../bin

clean:
		rm -f mFixNaN *.o
