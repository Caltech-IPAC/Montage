# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        04Oct05  Original SunOS Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
# WORKAROUND (issue #96): pre-C23 until local `svc_value()`/`url_encode()` declarations and coord.h/svc.h get real prototypes.
CFLAGS =	-g -O2 -std=gnu17 -I. -I../../../lib/include -I../../../Montage
LIBS   =	-L../../../lib -lsvc -lmtbl -lwcs -lcfitsio -lcoord -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mExecTG:	mExecTG.o
		$(CC) -o mExecTG mExecTG.o \
		../../../Montage/debugCheck.o   \
		$(LIBS)

install:
		cp mExecTG ../../../bin

clean:
		rm -f mExecTG *.o
