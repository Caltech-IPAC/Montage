# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        04Oct05  Original SunOS Makefile

CPPFLAGS +=	-I. -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../lib -lsvc -lmtbl -lcmd -lwcs -lcfitsio -lcoord -lboundaries -lm

all:		mExec

mExec: mExec.o ../../Montage/debugCheck.o ../../Montage/filePath.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mExec ../../bin

clean:
		rm -f mExec *.o
