# Filename: Makefile.SunOS

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------
# 1.0      John Good        15Dec04  Original Makefile

CPPFLAGS +=	-I. -I../../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../../lib -lwcs -lmtbl -lsvc

mArchiveExec: mArchiveExec.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mArchiveExec ../../../bin

clean:
		rm -f mArchiveExec *.o
