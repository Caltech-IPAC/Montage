CPPFLAGS += -I. -I.. -I../../lib/include -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   += -std=c99 -fPIC
LIBS      = -L../../lib -lsvc -lm

all: mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec

mHdrWWT: mHdrWWT.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHdrWWTExec: mHdrWWTExec.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mProjWWTExec: mProjWWTExec.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mPNGWWTExec: mPNGWWTExec.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec ../../bin

clean:
		rm -f mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec *.o
