.SUFFIXES:
.SUFFIXES: .c .o

CC     = gcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC
CFLAGS = -g -O2 -I. -I.. -I../../lib/include -Wall
LIBS   = -L../../lib -lsvc -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all: mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec

mHdrWWT:	mHdrWWT.o
		$(CC) -o mHdrWWT mHdrWWT.o $(LIBS)

mHdrWWTExec:	mHdrWWTExec.o
		$(CC) -o mHdrWWTExec mHdrWWTExec.o $(LIBS)

mProjWWTExec:	mProjWWTExec.o
		$(CC) -o mProjWWTExec mProjWWTExec.o $(LIBS)

mPNGWWTExec:	mPNGWWTExec.o
		$(CC) -o mPNGWWTExec mPNGWWTExec.o $(LIBS)

install:
		cp mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec ../../bin

clean:
		rm -f mHdrWWT mHdrWWTExec mProjWWTExec mPNGWWTExec *.o
