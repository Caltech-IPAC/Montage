# Filename: Makefile.Darwin

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        04Oct05  Original SunOS Makefile

CPPFLAGS +=	-I. -I../../../lib/include -I../../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../../lib -lsvc -lmtbl -lwcs -lcfitsio -lcoord -lm

mNotifyTG: mNotifyTG.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mNotifyTG ../../../bin

clean:
		rm -f mNotifyTG *.o
