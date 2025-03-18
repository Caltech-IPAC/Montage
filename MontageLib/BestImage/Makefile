# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------
# 1.0      John Good        14Feb05  Original Makefile

CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS      =	-L../../lib -lwcs -lmtbl -lm

mBestImage: mBestImage.o montageBestImage.o \
  ../util/checkWCS.o ../util/filePath.o ../util/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mBestImage ../../bin

clean:
		rm -f mBestImage *.o
