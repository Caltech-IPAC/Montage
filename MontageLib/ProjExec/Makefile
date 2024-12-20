.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
CPPFLAGS +=	-I. -I.. -I../../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99 -fPIC
LIBS   =	-L../../lib -lwcs -lmtbl -lwww -lcoord -ltwoplane -lcfitsio -lm

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

mProjExec: mProjExec.o montageProjExec.o \
  ../TANHdr/montageTANHdr.o \
  ../GetHdr/montageGetHdr.o \
  ../ProjectCube/montageProjectCube.o \
  ../Project/montageProject.o \
  ../ProjectQL/montageProjectQL.o \
  ../ProjectPP/montageProjectPP.o \
  ../util/checkFile.o ../util/filePath.o ../util/debugCheck.o \
  ../util/checkHdr.o ../util/checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp mProjExec ../../bin

clean:
		rm -f mProjExec *.o
