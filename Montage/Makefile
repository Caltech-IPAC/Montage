# Filename: Makefile

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 2.00     John Good        15may15  Added datacube utilities            
# 1.26     John Good        06Sep06  Added mTblSort                      
# 1.25     John Good        08Aug06  Added mDiffFitExec                  
# 1.24     John Good        03Feb06  Added mConvert                      
# 1.23     John Good        30May05  Added mPutHdr                       
# 1.22     John Good        13Feb05  Added mTileImage                       
# 1.21     John Good        30Jan05  Added mHdrtbl module (analog to mImgtbl
#                                    for header template files)
# 1.20     John Good        22Jan05  Modified MPI processing and changed
#                                    compiler to gcc
# 1.19     Daniel S. Katz   15Dec04  Added MPI versions: mProjExecMPI,
#                                    mFitExecMPI, mDiffExecMPI, mBgExecMPI,
#                                    mAddMPI, and mAddExecMPI
# 1.18     John Good        06Dec04  Changed to using BINS variable
#                                    and added mAddExec
# 1.17     John Good        08Oct04  Added mTileHdr
# 1.16     John Good        08Sep04  Moved mConcatFit to grid/Pegasus
#                                    and mMakeImg from contrib to here
# 1.15     John Good        08Jun04  Moved mRadHit/mRadHitExec to contrib
# 1.14     John Good        02Jun04  Added 64-bit addressing flag
# 1.13     John Good        27May04  Moved mDAGTbls, mNotify to contrib
#                                    Removed mSubtractImage (combined 
#                                    functionality with mDiff)
# 1.12     John Good        15Apr04  Added mTANHdr
# 1.11     John Good        18Mar04  Added table driven mode to mImgtbl
# 1.10     John Good        04Mar04  Added mRadHit/mRadHitExec
# 1.9      John Good        15Feb04  Added mShrink
# 1.8      John Good        27Jan04  Added mProjectPP
# 1.7      John Good        02Oct03  Added mConcatFit and mDAGTbls
# 1.6      John Good        26May03  Added checkHdr.o reference 
#                                    to mCoverageCheck
# 1.5      John Good        29Apr03  Added checkFile.o reference 
#                                    to mCoverageCheck
# 1.4      John Good        23Apr03  Added checkFile.o reference to mHdrCheck
# 1.3      John Good        25Mar03  Added checkFile.o reference to mProjExec
# 1.2      John Good        13Mar03  Updated mHdrCheck linkage and renamed
#                                    wcsCheck to checkWCS for consistency
# 1.1      John Good        13Mar03  Added mCoverageCheck, mHdrCheck, mGetHdr
#                                    as well as several checking functions
# 1.0      John Good        29Jan03  Original LINUX Makefile

.SUFFIXES:
.SUFFIXES: .c .o

CC     =	gcc
MPICC  =
CPPFLAGS +=	-I. -I../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   +=	-std=c99
LIBS   =	-L../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		   -lwww -lboundaries -lpixbounds -ltwoplane -lm

SBINS  =  mConvert mFlattenExec mHdrCheck mHdrtbl mTblSort mTileHdr mTileImage
MBINS  =  mProjExecMPI mFitExecMPI mDiffExecMPI mBgExecMPI mAddMPI  mAddExecMPI

BINS   = 	$(SBINS)


# uncomment the next two lines to build MPI modules
# MPICC  =	mpicc
# BINS = 		$(SBINS) $(MBINS)


.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c  $*.c

all:		$(BINS)

mAdd: mAdd.o filePath.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mAddCube: mAddCube.o filePath.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mAddMPI: mAddMPI.o filePath.o debugCheck.o checkHdr.o checkWCS.o
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LIBS)

mAddMPI.o: mAdd.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DMPI -o $@ -c $^

mAddExec:	mAddExec.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mAddExecMPI:	mAddExecMPI.o debugCheck.o checkHdr.o checkWCS.o
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LIBS)

mAddExecMPI.o:	mAddExec.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DMPI -o $@ -c $^

mBackground:	mBackground.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mBgExec:	mBgExec.o filePath.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mBgExecMPI:	mBgExecMPI.o filePath.o
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LIBS)

mBgExecMPI.o:	mBgExec.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DMPI -o $@ -c $^

mBgModel:	mBgModel.o debugCheck.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mConvert:	mConvert.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mCoverageCheck:	mCoverageCheck.o checkHdr.o checkWCS.o checkFile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mDiff:		mDiff.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mDiffExec:	mDiffExec.o filePath.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mDiffExecMPI:	mDiffExecMPI.o filePath.o checkHdr.o checkWCS.o
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LIBS)

mDiffExecMPI.o:	mDiffExec.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DMPI -o $@ -c $^

mDiffFitExec:	mDiffFitExec.o filePath.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mFitExec:	mFitExec.o filePath.o checkFile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mFitExecMPI:	mFitExecMPI.o filePath.o checkFile.o
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LIBS)

mFitExecMPI.o:	mFitExec.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DMPI -o $@ -c $^

mFitplane:	mFitplane.o debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mFlattenExec:	mFlattenExec.o filePath.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mGetHdr:	mGetHdr.o checkFile.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHdrCheck:	mHdrCheck.o checkHdr.o checkWCS.o checkFile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mHdrtbl:	mHdrtbl.o get_hfiles.o get_hhdr.o print_hrec.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mImgtbl:	mImgtbl.o get_files.o get_hdr.o print_rec.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mMakeHdr:	mMakeHdr.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mMakeImg:	mMakeImg.o ltqnorm.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mOverlaps:	mOverlaps.o checkWCS.o filePath.o debugCheck.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mProjExec:	mProjExec.o filePath.o checkHdr.o checkWCS.o checkFile.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mProjExecMPI:	mProjExecMPI.o filePath.o checkHdr.o checkWCS.o checkFile.o
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LIBS)

mProjExecMPI.o:	mProjExec.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DMPI -o $@ -c $^

mProject:	mProject.o overlapArea.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mProjectCube:	mProjectCube.o overlapArea.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mProjectPP:	mProjectPP.o overlapAreaPP.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mPutHdr:	mPutHdr.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mShrink:	mShrink.o debugCheck.o checkHdr.o checkWCS.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mShrinkCube:	mShrinkCube.o debugCheck.o checkHdr.o checkWCS.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mSubCube:	mSubCube.o checkHdr.o checkWCS.o subCube.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mSubimage:	mSubimage.o checkHdr.o checkWCS.o subImage.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mSubset:	mSubset.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mTANHdr:	mTANHdr.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mTblSort:	mTblSort.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mTileHdr:	mTileHdr.o debugCheck.o checkHdr.o checkWCS.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mTileImage:	mTileImage.o checkHdr.o checkWCS.o subImage.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install:
		cp $(BINS) ../bin

clean:
		rm -f $(BINS) *.o
