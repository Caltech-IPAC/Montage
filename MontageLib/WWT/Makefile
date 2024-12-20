.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc
CPPFLAGS += -I. -I.. -I../../lib/include -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   += -std=c99 -fPIC
LIBS   = -L../../lib -lsvc -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all: mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec

mHdrWWT: mHdrWWT.o
	$(CC) $(LDFLAGS) -o mHdrWWT mHdrWWT.o $(LIBS)

mHdrWWTExec: mHdrWWTExec.o
	$(CC) $(LDFLAGS) -o mHdrWWTExec mHdrWWTExec.o $(LIBS)

mProjWWTExec: mProjWWTExec.o
	$(CC) $(LDFLAGS) -o mProjWWTExec mProjWWTExec.o $(LIBS)

mPNGWWTExec: mPNGWWTExec.o
	$(CC) $(LDFLAGS) -o mPNGWWTExec mPNGWWTExec.o $(LIBS)

install:
		cp mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec ../../bin

clean:
		rm -f mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec *.o
