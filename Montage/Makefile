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
CFLAGS =	-g -O2 -I. -I../lib/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -std=c99
LIBS   =	-L../lib -lwcs -lcfitsio -lcoord -lmtbl -lsvc \
		   -lwww -lboundaries -lpixbounds -ltwoplane -lm

SBINS  =  mConvert mFlattenExec mHdrCheck mHdrtbl mTblSort mTileHdr mTileImage
MBINS  =  mProjExecMPI mFitExecMPI mDiffExecMPI mBgExecMPI mAddMPI  mAddExecMPI

BINS   = 	$(SBINS)


# uncomment the next two lines to build MPI modules
# MPICC  =	mpicc
# BINS = 		$(SBINS) $(MBINS)


.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		$(BINS)

mAdd:		mAdd.o filePath.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mAdd mAdd.o filePath.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mAddCube:	mAddCube.o filePath.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mAddCube mAddCube.o filePath.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mAddMPI:	mAddMPI.o filePath.o debugCheck.o checkHdr.o checkWCS.o
		$(MPICC) -o mAddMPI mAddMPI.o filePath.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mAddMPI.o:	mAdd.c
		$(MPICC) -o mAddMPI.o -DMPI $(CFLAGS) -c mAdd.c

mAddExec:	mAddExec.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mAddExec mAddExec.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mAddExecMPI:	mAddExecMPI.o debugCheck.o checkHdr.o checkWCS.o
		$(MPICC) -o mAddExecMPI mAddExecMPI.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mAddExecMPI.o:	mAddExec.c
		$(MPICC) -o mAddExecMPI.o -DMPI $(CFLAGS) -c mAddExec.c

mBackground:	mBackground.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mBackground mBackground.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mBgExec:	mBgExec.o filePath.o
		$(CC) -o mBgExec mBgExec.o filePath.o $(LIBS)

mBgExecMPI:	mBgExecMPI.o filePath.o
		$(MPICC) -o mBgExecMPI mBgExecMPI.o filePath.o $(LIBS)

mBgExecMPI.o:	mBgExec.c
		$(MPICC) -o mBgExecMPI.o -DMPI $(CFLAGS) -c mBgExec.c

mBgModel:	mBgModel.o debugCheck.o 
		$(CC) -o mBgModel mBgModel.o debugCheck.o $(LIBS)

mConvert:	mConvert.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mConvert mConvert.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mCoverageCheck:	mCoverageCheck.o checkHdr.o checkWCS.o checkFile.o
		$(CC) -o mCoverageCheck mCoverageCheck.o checkHdr.o \
		checkWCS.o checkFile.o                              \
		$(LIBS)

mDiff:		mDiff.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mDiff mDiff.o debugCheck.o checkHdr.o checkWCS.o \
		$(LIBS)

mDiffExec:	mDiffExec.o filePath.o checkHdr.o checkWCS.o
		$(CC) -o mDiffExec mDiffExec.o filePath.o checkHdr.o \
		checkWCS.o $(LIBS)

mDiffExecMPI:	mDiffExecMPI.o filePath.o checkHdr.o checkWCS.o
		$(MPICC) -o mDiffExecMPI mDiffExecMPI.o filePath.o checkHdr.o \
		checkWCS.o $(LIBS)

mDiffExecMPI.o:	mDiffExec.c
		$(MPICC) -o mDiffExecMPI.o -DMPI $(CFLAGS) -c mDiffExec.c

mDiffFitExec:	mDiffFitExec.o filePath.o checkHdr.o checkWCS.o
		$(CC) -o mDiffFitExec mDiffFitExec.o filePath.o checkHdr.o \
		checkWCS.o $(LIBS)

mFitExec:	mFitExec.o filePath.o checkFile.o
		$(CC) -o mFitExec mFitExec.o filePath.o checkFile.o $(LIBS)

mFitExecMPI:	mFitExecMPI.o filePath.o checkFile.o
		$(MPICC) -o mFitExecMPI mFitExecMPI.o filePath.o \
		checkFile.o $(LIBS)

mFitExecMPI.o:	mFitExec.c
		$(MPICC) -o mFitExecMPI.o -DMPI $(CFLAGS) -c mFitExec.c

mFitplane:	mFitplane.o debugCheck.o
		$(CC) -o mFitplane mFitplane.o debugCheck.o $(LIBS)

mFlattenExec:	mFlattenExec.o filePath.o
		$(CC) -o mFlattenExec mFlattenExec.o filePath.o $(LIBS)

mGetHdr:	mGetHdr.o checkFile.o checkHdr.o checkWCS.o
		$(CC) -o mGetHdr checkFile.o mGetHdr.o checkHdr.o checkWCS.o \
		$(LIBS)

mHdrCheck:	mHdrCheck.o checkHdr.o checkWCS.o checkFile.o
		$(CC) -o mHdrCheck mHdrCheck.o checkHdr.o checkWCS.o \
		checkFile.o $(LIBS)

mHdrtbl:	mHdrtbl.o get_hfiles.o get_hhdr.o print_hrec.o checkWCS.o
		$(CC) -g -O2 -o mHdrtbl mHdrtbl.o get_hfiles.o \
		get_hhdr.o print_hrec.o checkWCS.o $(LIBS)

mImgtbl:	mImgtbl.o get_files.o get_hdr.o print_rec.o checkWCS.o
		$(CC) -g -O2 -o mImgtbl mImgtbl.o get_files.o \
		get_hdr.o print_rec.o checkWCS.o $(LIBS)

mMakeHdr:	mMakeHdr.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mMakeHdr mMakeHdr.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mMakeImg:	mMakeImg.o ltqnorm.o
		$(CC) -o mMakeImg mMakeImg.o ltqnorm.o $(LIBS)

mOverlaps:	mOverlaps.o checkWCS.o filePath.o debugCheck.o
		$(CC) -o mOverlaps mOverlaps.o checkWCS.o filePath.o \
		debugCheck.o $(LIBS)

mProjExec:	mProjExec.o filePath.o checkHdr.o checkWCS.o checkFile.o
		$(CC) -o mProjExec mProjExec.o filePath.o checkHdr.o \
		checkWCS.o checkFile.o $(LIBS)

mProjExecMPI:	mProjExecMPI.o filePath.o checkHdr.o checkWCS.o checkFile.o
		$(MPICC) -o mProjExecMPI mProjExecMPI.o filePath.o checkHdr.o \
		checkWCS.o checkFile.o $(LIBS)

mProjExecMPI.o:	mProjExec.c
		$(MPICC) -o mProjExecMPI.o -DMPI $(CFLAGS) -c mProjExec.c

mProject:	mProject.o overlapArea.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mProject mProject.o overlapArea.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mProjectCube:	mProjectCube.o overlapArea.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mProjectCube mProjectCube.o overlapArea.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mProjectPP:	mProjectPP.o overlapAreaPP.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mProjectPP mProjectPP.o overlapAreaPP.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mPutHdr:	mPutHdr.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mPutHdr mPutHdr.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mShrink:	mShrink.o debugCheck.o checkHdr.o checkWCS.o 
		$(CC) -o mShrink mShrink.o debugCheck.o checkHdr.o checkWCS.o $(LIBS)

mShrinkCube:	mShrinkCube.o debugCheck.o checkHdr.o checkWCS.o 
		$(CC) -o mShrinkCube mShrinkCube.o debugCheck.o checkHdr.o checkWCS.o $(LIBS)

mSubCube:	mSubCube.o checkHdr.o checkWCS.o subCube.o
		$(CC) -o mSubCube mSubCube.o checkHdr.o \
		checkWCS.o subCube.o $(LIBS)

mSubimage:	mSubimage.o checkHdr.o checkWCS.o subImage.o
		$(CC) -o mSubimage mSubimage.o checkHdr.o \
		checkWCS.o subImage.o $(LIBS)

mSubset:	mSubset.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mSubset mSubset.o debugCheck.o checkHdr.o \
		checkWCS.o $(LIBS)

mTANHdr:	mTANHdr.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mTANHdr mTANHdr.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mTblSort:	mTblSort.o 
		$(CC) -o mTblSort mTblSort.o $(LIBS)

mTileHdr:	mTileHdr.o debugCheck.o checkHdr.o checkWCS.o
		$(CC) -o mTileHdr mTileHdr.o debugCheck.o \
		checkHdr.o checkWCS.o $(LIBS)

mTileImage:	mTileImage.o checkHdr.o checkWCS.o subImage.o
		$(CC) -o mTileImage mTileImage.o \
		checkHdr.o checkWCS.o subImage.o $(LIBS)

install:
		cp $(BINS) ../bin

clean:
		rm -f $(BINS) *.o
