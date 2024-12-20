# Filename: Makefile.SunOS

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------
# 1.0      John Good        15Dec04  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lbz2

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mArchiveExec: mArchiveExec.o montageArchiveExec.o \
  ../ArchiveGet/montageArchiveGet.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mArchiveExec ../../bin

clean:
		rm -f mArchiveExec *.o
