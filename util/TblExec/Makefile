# Filename: Makefile.SunOS

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        21Sep07  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS   =	-L../../lib -lsvc -lmtbl -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mTblExec: mTblExec.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mTblExec ../../bin

clean:
		rm -f mTblExec *.o
