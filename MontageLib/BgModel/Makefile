.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lmtbl -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mBgModel: mBgModel.o montageBgModel.o ../util/debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mBgModel ../../bin

clean:
		rm -f mBgModel *.o
