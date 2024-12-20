# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.0      John Good        20Feb07  Original Darwin Makefile

CPPFLAGS +=	-I. -I./rtree -I../../lib/include -I../../Montage -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBS      =	-Lrtree -lrtree -L../../lib -lcmd -lwcs -lcoord -lmtbl -lcfitsio -lm

mSearch: mSearch.o \
  ../../Montage/debugCheck.o ../../Montage/checkFile.o ../../Montage/checkWCS.o
	$(MAKE) -C rtree
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mSearch ../../bin

clean:
		$(MAKE) -C rtree clean
		rm -f mSearch *.o
