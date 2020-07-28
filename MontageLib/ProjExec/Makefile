.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CFLAGS =	-g -O2 -I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99 -fPIC -Wall
LIBS   =	-L../../lib -lwcs -lmtbl -lwww -lcoord -ltwoplane -lcfitsio -lm

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

mProjExec:		mProjExec.o montageProjExec.o
		$(CC) -o mProjExec mProjExec.o montageProjExec.o \
		../TANHdr/montageTANHdr.o \
		../GetHdr/montageGetHdr.o \
		../ProjectCube/montageProjectCube.o \
		../Project/montageProject.o \
		../ProjectQL/montageProjectQL.o \
		../ProjectPP/montageProjectPP.o \
		../util/checkFile.o ../util/filePath.o ../util/debugCheck.o ../util/checkHdr.o \
		../util/checkWCS.o $(LIBS)

install:
		cp mProjExec ../../bin

clean:
		rm -f mProjExec *.o
