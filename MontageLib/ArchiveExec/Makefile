# Filename: Makefile.SunOS

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------
# 1.0      John Good        15Dec04  Original Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
# WORKAROUND (issue #96): pre-C23 until montageArchiveExec.c's local `svc_value()` declaration gets a real prototype.
CFLAGS =	-g -O2 -std=gnu17 -I. -I.. -I../../lib/include -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lbz2

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mArchiveExec:	mArchiveExec.o montageArchiveExec.o ../ArchiveGet/montageArchiveGet.o
		$(CC) -o mArchiveExec mArchiveExec.o montageArchiveExec.o ../ArchiveGet/montageArchiveGet.o $(LIBS)

install:
		cp mArchiveExec ../../bin

clean:
		rm -f mArchiveExec *.o
