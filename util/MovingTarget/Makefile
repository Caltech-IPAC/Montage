# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        10Apr15  Original LINUX Makefile (from mSearch)

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I./rtree -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   =	-Lrtree -lrtree -L../../lib -lcmd -lwcs -lcoord -lmtbl -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mMovingTarget:	mMovingTarget.o \
  ../../Montage/debugCheck.o ../../Montage/checkFile.o ../../Montage/checkWCS.o
	$(MAKE) -C rtree
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mMovingTarget ../../bin

clean:
		$(MAKE) -C rtree clean
		rm -f mMovingTarget *.o
