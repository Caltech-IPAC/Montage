# Filename: Makefile.LINUX

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        15Nov15  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS   =	-L../../lib -lsvc -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mCatSearch:		mCatSearch.o
		$(CC) $(LDFLAGS) -o mCatSearch mCatSearch.o $(LIBS)

install:
		cp mCatSearch ../../bin

clean:
		rm -f mCatSearch *.o
