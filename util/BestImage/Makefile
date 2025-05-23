# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------
# 1.0      John Good        14Feb05  Original Makefile

CPPFLAGS +=	-I. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-L../../lib -lwcs -lmtbl -lsvc -lm

mBestImage: mBestImage.o \
  ../../Montage/checkWCS.o ../../Montage/filePath.o ../../Montage/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mBestImage ../../bin

clean:
		rm -f mBestImage *.o
