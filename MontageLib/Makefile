UNAME_S := $(shell uname -s)

SO_FLAG := 

ifeq ($(UNAME_S),Darwin)
   SO_FLAG := -undefined dynamic_lookup
endif

all: pgm lib

pgm: 
		(cd util;                          make)
		(cd Add;           ./Configure.sh; make; make install)
		(cd AddCube;       ./Configure.sh; make; make install)
		(cd Background;    ./Configure.sh; make; make install)
		(cd BestImage;                     make; make install)
		(cd BgModel;                       make; make install)
		(cd CoverageCheck; ./Configure.sh; make; make install)
		(cd Diff;          ./Configure.sh; make; make install)
		(cd Examine;       ./Configure.sh; make; make install)
		(cd Fitplane;                      make; make install)
		(cd FixNaN;        ./Configure.sh; make; make install)
		(cd GetHdr;                        make; make install)
		(cd Hdr;           ./Configure.sh; make; make install)
		(cd Histogram;                     make; make install)
		(cd Imgtbl;        ./Configure.sh; make; make install)
		(cd MakeHdr;       ./Configure.sh; make; make install)
		(cd MakeImg;       ./Configure.sh; make; make install)
		(cd Overlaps;      ./Configure.sh; make; make install)
		(cd Project;       ./Configure.sh; make; make install)
		(cd ProjectCube;   ./Configure.sh; make; make install)
		(cd ProjectPP;     ./Configure.sh; make; make install)
		(cd ProjectQL;     ./Configure.sh; make; make install)
		(cd PutHdr;                        make; make install)
		(cd ShrinkCube;                    make; make install)
		(cd Shrink;                        make; make install)
		(cd SubCube;       ./Configure.sh; make; make install)
		(cd Subimage;      ./Configure.sh; make; make install)
		(cd Subset;        ./Configure.sh; make; make install)
		(cd Transpose;     ./Configure.sh; make; make install)
		(cd WWT;           ./Configure.sh; make; make install)
		(cd Viewer;        ./Configure.sh; make; make install)

lib:
		rm -f libmontage.a libmontage.so
		ar q  libmontage.a \
			util/checkFile.o util/checkHdr.o util/checkWCS.o \
			util/debugCheck.o util/filePath.o \
			Add/montageAdd.o \
			AddCube/montageAddCube.o \
			Background/montageBackground.o \
			BestImage/montageBestImage.o \
			BgModel/montageBgModel.o \
			CoverageCheck/montageCoverageCheck.o \
			Diff/montageDiff.o \
			Examine/montageExamine.o \
			Fitplane/montageFitplane.o \
			FixNaN/montageFixNaN.o \
			GetHdr/montageGetHdr.o \
			Hdr/montageHdr.o \
			Histogram/montageHistogram.o \
			Imgtbl/montageImgtbl.o \
			MakeHdr/montageMakeHdr.o \
			MakeImg/montageMakeImg.o \
			Overlaps/montageOverlaps.o \
			ProjectCube/montageProjectCube.o \
			Project/montageProject.o \
			ProjectPP/montageProjectPP.o \
			ProjectQL/montageProjectQL.o \
			PutHdr/montagePutHdr.o \
			ShrinkCube/montageShrinkCube.o \
			Shrink/montageShrink.o \
			SubCube/montageSubCube.o \
			Subimage/montageSubimage.o \
			Subset/montageSubset.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o
		gcc -shared $(SO_FLAG) -o libmontage.so \
			util/checkFile.o util/checkHdr.o util/checkWCS.o \
			util/debugCheck.o util/filePath.o \
			Add/montageAdd.o \
			AddCube/montageAddCube.o \
			Background/montageBackground.o \
			BestImage/montageBestImage.o \
			BgModel/montageBgModel.o \
			CoverageCheck/montageCoverageCheck.o \
			Diff/montageDiff.o \
			Examine/montageExamine.o \
			Fitplane/montageFitplane.o \
			FixNaN/montageFixNaN.o \
			GetHdr/montageGetHdr.o \
			Hdr/montageHdr.o \
			Histogram/montageHistogram.o \
			Imgtbl/montageImgtbl.o \
			MakeHdr/montageMakeHdr.o \
			MakeImg/montageMakeImg.o \
			Overlaps/montageOverlaps.o \
			ProjectCube/montageProjectCube.o \
			Project/montageProject.o \
			ProjectPP/montageProjectPP.o \
			ProjectQL/montageProjectQL.o \
			PutHdr/montagePutHdr.o \
			ShrinkCube/montageShrinkCube.o \
			Shrink/montageShrink.o \
			SubCube/montageSubCube.o \
			Subimage/montageSubimage.o \
			Subset/montageSubset.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o

doc:
			gcc -o mLibDoc mLibDoc.c
			mLibDoc Add
			mLibDoc AddCube
			mLibDoc Background
			mLibDoc BestImage
			mLibDoc BgModel
			mLibDoc CoverageCheck
			mLibDoc Diff
			mLibDoc Examine
			mLibDoc Fitplane
			mLibDoc FixNaN
			mLibDoc GetHdr
			mLibDoc Hdr
			mLibDoc Histogram
			mLibDoc Imgtbl
			mLibDoc MakeHdr
			mLibDoc MakeImg
			mLibDoc Overlaps
			mLibDoc ProjectCube
			mLibDoc Project
			mLibDoc ProjectPP
			mLibDoc ProjectQL
			mLibDoc PutHdr
			mLibDoc ShrinkCube
			mLibDoc Shrink
			mLibDoc SubCube
			mLibDoc Subimage
			mLibDoc Subset
			mLibDoc Transpose
			mLibDoc Viewer

clean:
		(cd util;          make clean)
		(cd Add;           make clean)
		(cd AddCube;       make clean)
		(cd Background;    make clean)
		(cd BestImage;     make clean)
		(cd BgModel;       make clean)
		(cd CoverageCheck; make clean)
		(cd Diff;          make clean)
		(cd Examine;       make clean)
		(cd Fitplane;      make clean)
		(cd FixNaN;        make clean)
		(cd GetHdr;        make clean)
		(cd Hdr;           make clean)
		(cd Histogram;     make clean)
		(cd Imgtbl;        make clean)
		(cd MakeHdr;       make clean)
		(cd MakeImg;       make clean)
		(cd Overlaps;      make clean)
		(cd ProjectCube;   make clean)
		(cd Project;       make clean)
		(cd ProjectPP;     make clean)
		(cd ProjectQL;     make clean)
		(cd PutHdr;        make clean)
		(cd ShrinkCube;    make clean)
		(cd Shrink;        make clean)
		(cd SubCube;       make clean)
		(cd Subimage;      make clean)
		(cd Subset;        make clean)
		(cd Transpose;     make clean)
		(cd WWT;           make clean)
		(cd Viewer;        make clean)
		rm -f libmontage.a libmontage.so
