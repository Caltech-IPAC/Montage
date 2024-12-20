CPPFLAGS +=	-I. -I../../MontageLib -I../../lib/include \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS      =	-L../../lib -lwcs -lcfitsio -lpixbounds -lm

all: mHiPSTiles

mHiPSTiles: mHiPSTiles.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mHiPSTiles ../../bin
clean:
		rm -f mHiPSTiles *.o
