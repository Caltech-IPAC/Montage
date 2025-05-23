# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        04Oct05  Original SunOS Makefile

CPPFLAGS +=	-I. -I../../../lib/include -I../../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../../lib -lsvc -lmtbl -lwcs -lcfitsio -lcoord -lm

mExecTG: mExecTG.o ../../../Montage/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mExecTG ../../../bin

clean:
		rm -f mExecTG *.o
