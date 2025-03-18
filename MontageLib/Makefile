UNAME_S := $(shell uname -s)

SO_FLAG := 

ifeq ($(UNAME_S),Darwin)
   SO_FLAG := -undefined dynamic_lookup
endif

all: pgm lib pythonlibs

pgm: 
		(cd util          && $(MAKE))
		(cd Add           && $(MAKE) && $(MAKE) install)
		(cd AddCube       && $(MAKE) && $(MAKE) install)
		(cd  AddMem       && $(MAKE) && $(MAKE) install)
		(cd Background    && $(MAKE) && $(MAKE) install)
		(cd BestImage     && $(MAKE) && $(MAKE) install)
		(cd BgModel       && $(MAKE) && $(MAKE) install)
		(cd Coord         && $(MAKE) && $(MAKE) install)
		(cd CoverageCheck && $(MAKE) && $(MAKE) install)
		(cd Diff          && $(MAKE) && $(MAKE) install)
		(cd Examine       && $(MAKE) && $(MAKE) install)
		(cd Fitplane      && $(MAKE) && $(MAKE) install)
		(cd DiffFit       && $(MAKE) && $(MAKE) install)
		(cd FixNaN        && $(MAKE) && $(MAKE) install)
		(cd Flatten       && $(MAKE) && $(MAKE) install)
		(cd GetHdr        && $(MAKE) && $(MAKE) install)
		(cd Hdr           && $(MAKE) && $(MAKE) install)
		(cd Histogram     && $(MAKE) && $(MAKE) install)
		(cd Imgtbl        && $(MAKE) && $(MAKE) install)
		(cd ArchiveList   && $(MAKE) && $(MAKE) install)
		(cd ArchiveGet    && $(MAKE) && $(MAKE) install)
		(cd ArchiveExec   && $(MAKE) && $(MAKE) install)
		(cd MakeHdr       && $(MAKE) && $(MAKE) install)
		(cd MakeImg       && $(MAKE) && $(MAKE) install)
		(cd Mask          && $(MAKE) && $(MAKE) install)
		(cd Overlaps      && $(MAKE) && $(MAKE) install)
		(cd Project       && $(MAKE) && $(MAKE) install)
		(cd ProjectCube   && $(MAKE) && $(MAKE) install)
		(cd ProjectPP     && $(MAKE) && $(MAKE) install)
		(cd ProjectQL     && $(MAKE) && $(MAKE) install)
		(cd PutHdr        && $(MAKE) && $(MAKE) install)
		(cd ShrinkCube    && $(MAKE) && $(MAKE) install)
		(cd Shrink        && $(MAKE) && $(MAKE) install)
		(cd SubCube       && $(MAKE) && $(MAKE) install)
		(cd SubHdr        && $(MAKE) && $(MAKE) install)
		(cd Subimage      && $(MAKE) && $(MAKE) install)
		(cd Subset        && $(MAKE) && $(MAKE) install)
		(cd TANHdr        && $(MAKE) && $(MAKE) install)
		(cd TileImage     && $(MAKE) && $(MAKE) install)
		(cd Transpose     && $(MAKE) && $(MAKE) install)
		(cd WWT           && $(MAKE) && $(MAKE) install)
		(cd Viewer        && $(MAKE) && $(MAKE) install)
		(cd DiffExec      && $(MAKE) && $(MAKE) install)
		(cd BgExec        && $(MAKE) && $(MAKE) install)
		(cd DiffFitExec   && $(MAKE) && $(MAKE) install)
		(cd FitExec       && $(MAKE) && $(MAKE) install)
		(cd ProjExec      && $(MAKE) && $(MAKE) install)

lib: pgm
		rm -f libmontage.a libmontage.so libmontagepy.so
		ar q  libmontage.a \
			util/checkFile.o util/checkHdr.o util/checkWCS.o \
			util/debugCheck.o util/filePath.o \
			Add/montageAdd.o \
			AddCube/montageAddCube.o \
			AddMem/montageAddMem.o \
			ArchiveList/montageArchiveList.o \
			ArchiveGet/montageArchiveGet.o \
			ArchiveExec/montageArchiveExec.o \
			Background/montageBackground.o \
			BestImage/montageBestImage.o \
			BgModel/montageBgModel.o \
			Coord/montageCoord.o \
			CoverageCheck/montageCoverageCheck.o \
			DiffFit/montageDiffFit.o \
			Diff/montageDiff.o \
			Examine/montageExamine.o \
			Fitplane/montageFitplane.o \
			FixNaN/montageFixNaN.o \
			Flatten/montageFlatten.o \
			GetHdr/montageGetHdr.o \
			Hdr/montageHdr.o \
			Histogram/montageHistogram.o \
			Imgtbl/montageImgtbl.o \
			MakeHdr/montageMakeHdr.o \
			MakeImg/montageMakeImg.o \
			Mask/montageMask.o \
			Overlaps/montageOverlaps.o \
			ProjectCube/montageProjectCube.o \
			Project/montageProject.o \
			ProjectPP/montageProjectPP.o \
			ProjectQL/montageProjectQL.o \
			PutHdr/montagePutHdr.o \
			ShrinkCube/montageShrinkCube.o \
			Shrink/montageShrink.o \
			SubCube/montageSubCube.o \
			SubHdr/montageSubHdr.o \
			Subimage/montageSubimage.o \
			Subset/montageSubset.o \
			TANHdr/montageTANHdr.o \
			TileImage/montageTileImage.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o \
			Viewer/mViewer_boundingbox.o \
			BgExec/montageBgExec.o \
			DiffExec/montageDiffExec.o \
			FitExec/montageFitExec.o \
			DiffFitExec/montageDiffFitExec.o \
			ProjExec/montageProjExec.o 
		gcc -shared $(SO_FLAG) -o libmontage.so \
			util/checkFile.o util/checkHdr.o util/checkWCS.o \
			util/debugCheck.o util/filePath.o \
			Add/montageAdd.o \
			AddCube/montageAddCube.o \
			AddMem/montageAddMem.o \
			ArchiveList/montageArchiveList.o \
			ArchiveGet/montageArchiveGet.o \
			ArchiveExec/montageArchiveExec.o \
			Background/montageBackground.o \
			BestImage/montageBestImage.o \
			BgModel/montageBgModel.o \
			Coord/montageCoord.o \
			CoverageCheck/montageCoverageCheck.o \
			DiffFit/montageDiffFit.o \
			Diff/montageDiff.o \
			Examine/montageExamine.o \
			Fitplane/montageFitplane.o \
			FixNaN/montageFixNaN.o \
			Flatten/montageFlatten.o \
			GetHdr/montageGetHdr.o \
			Hdr/montageHdr.o \
			Histogram/montageHistogram.o \
			Imgtbl/montageImgtbl.o \
			MakeHdr/montageMakeHdr.o \
			MakeImg/montageMakeImg.o \
			Mask/montageMask.o \
			Overlaps/montageOverlaps.o \
			ProjectCube/montageProjectCube.o \
			Project/montageProject.o \
			ProjectPP/montageProjectPP.o \
			ProjectQL/montageProjectQL.o \
			PutHdr/montagePutHdr.o \
			ShrinkCube/montageShrinkCube.o \
			Shrink/montageShrink.o \
			SubCube/montageSubCube.o \
			SubHdr/montageSubHdr.o \
			Subimage/montageSubimage.o \
			Subset/montageSubset.o \
			TANHdr/montageTANHdr.o \
			TileImage/montageTileImage.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o \
			Viewer/mViewer_boundingbox.o \
			BgExec/montageBgExec.o \
			DiffExec/montageDiffExec.o \
			FitExec/montageFitExec.o \
			DiffFitExec/montageDiffFitExec.o \
			ProjExec/montageProjExec.o 

pythonlibs: pgm
		rm -f ../python/MontagePy/lib/*.o
		mkdir -p ../python/MontagePy/lib
		cp util/checkFile.o util/checkHdr.o util/checkWCS.o \
			util/debugCheck.o util/filePath.o \
			Add/montageAdd.o \
			AddCube/montageAddCube.o \
			AddMem/montageAddMem.o \
			ArchiveList/montageArchiveList.o \
			ArchiveGet/montageArchiveGet.o \
			ArchiveExec/montageArchiveExec.o \
			Background/montageBackground.o \
			BestImage/montageBestImage.o \
			BgModel/montageBgModel.o \
			Coord/montageCoord.o \
			CoverageCheck/montageCoverageCheck.o \
			DiffFit/montageDiffFit.o \
			Diff/montageDiff.o \
			Examine/montageExamine.o \
			Fitplane/montageFitplane.o \
			FixNaN/montageFixNaN.o \
			Flatten/montageFlatten.o \
			GetHdr/montageGetHdr.o \
			Hdr/montageHdr.o \
			Histogram/montageHistogram.o \
			Imgtbl/montageImgtbl.o \
			MakeHdr/montageMakeHdr.o \
			MakeImg/montageMakeImg.o \
			Mask/montageMask.o \
			Overlaps/montageOverlaps.o \
			ProjectCube/montageProjectCube.o \
			Project/montageProject.o \
			ProjectPP/montageProjectPP.o \
			ProjectQL/montageProjectQL.o \
			PutHdr/montagePutHdr.o \
			ShrinkCube/montageShrinkCube.o \
			Shrink/montageShrink.o \
			SubCube/montageSubCube.o \
			SubHdr/montageSubHdr.o \
			Subimage/montageSubimage.o \
			Subset/montageSubset.o \
			TANHdr/montageTANHdr.o \
			TileImage/montageTileImage.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o \
			Viewer/mViewer_boundingbox.o \
			BgExec/montageBgExec.o \
			DiffExec/montageDiffExec.o \
			FitExec/montageFitExec.o \
			DiffFitExec/montageDiffFitExec.o \
			ProjExec/montageProjExec.o \
		../python/MontagePy/lib


                # We need to collect all the library .o files for the Extension build.
		# The last two are special; the .libs subdirectory contain versions of the
		# .o files compiled with -fPIC.

		find ../lib/src/bzip2-1.0.6                -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/cfitsio-3.49               -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/cmd                        -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/coord                      -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/mtbl                       -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/svc                        -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/json                       -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/boundaries                 -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/pixbounds                  -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/www                        -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/wcssubs3.9.0_montage       -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/two_plane_v1.1             -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/lodepng_20140823           -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/jpeg-8b/.libs              -name \*.o -exec cp {} ../python/MontagePy/lib \;
		find ../lib/src/freetype-2.9.1/objs/.libs  -name \*.o -exec cp {} ../python/MontagePy/lib \;
		

                # These .o files were for executables and contain main()

		rm ../python/MontagePy/lib/bzip2recover.o
		rm ../python/MontagePy/lib/bzip2.o

		
                # There are two libraries with conflicting .o file names

		cp ../lib/src/two_plane_v1.1/distort.o       ../python/MontagePy/lib/two_plane_distort.o
		cp ../lib/src/wcssubs3.9.0_montage/distort.o ../python/MontagePy/lib/distort.o

doc:
			gcc -o mLibDoc mLibDoc.c
			mLibDoc Add
			mLibDoc AddCube
			mLibDoc AddMem
			mLibDoc ArchiveList
			mLibDoc ArchiveGet
			mLibDoc ArchiveExec
			mLibDoc Background
			mLibDoc BestImage
			mLibDoc BgModel
			mLibDoc Coord
			mLibDoc CoverageCheck
			mLibDoc Diff
			mLibDoc DiffFit
			mLibDoc Examine
			mLibDoc Fitplane
			mLibDoc FixNaN
			mLibDoc Flatten
			mLibDoc GetHdr
			mLibDoc Hdr
			mLibDoc Histogram
			mLibDoc Imgtbl
			mLibDoc MakeHdr
			mLibDoc MakeImg
			mLibDoc Mask
			mLibDoc Overlaps
			mLibDoc ProjectCube
			mLibDoc Project
			mLibDoc ProjectPP
			mLibDoc ProjectQL
			mLibDoc PutHdr
			mLibDoc ShrinkCube
			mLibDoc Shrink
			mLibDoc SubCube
			mLibDoc SubHdr
			mLibDoc Subimage
			mLibDoc Subset
			mLibDoc TANHdr
			mLibDoc TileImage
			mLibDoc Transpose
			mLibDoc Viewer
			mLibDoc BgExec
			mLibDoc DiffExec
			mLibDoc FitExec
			mLibDoc DiffFitExec
			mLibDoc ProjExec

jupyter:
			gcc -o mJupyter mJupyter.c -I../lib/include -L ../lib -lsvc
			mJupyter Add
			mJupyter AddCube
			mJupyter AddMem
			mJupyter ArchiveList
			mJupyter ArchiveGet
			mJupyter ArchiveExec
			mJupyter Background
			mJupyter BestImage
			mJupyter BgModel
			mJupyter Coord
			mJupyter CoverageCheck
			mJupyter Diff
			mJupyter DiffFit
			mJupyter Examine
			mJupyter Fitplane
			mJupyter FixNaN
			mJupyter Flatten
			mJupyter GetHdr
			mJupyter Hdr
			mJupyter Histogram
			mJupyter Imgtbl
			mJupyter MakeHdr
			mJupyter MakeImg
			mJupyter Mask
			mJupyter Overlaps
			mJupyter ProjectCube
			mJupyter Project
			mJupyter ProjectPP
			mJupyter ProjectQL
			mJupyter PutHdr
			mJupyter ShrinkCube
			mJupyter Shrink
			mJupyter SubCube
			mJupyter SubHdr
			mJupyter Subimage
			mJupyter Subset
			mJupyter TANHdr
			mJupyter TileImage
			mJupyter Transpose
			mJupyter Viewer
			mJupyter BgExec
			mJupyter DiffExec
			mJupyter FitExec
			mJupyter DiffFitExec
			mJupyter ProjExec

clean:
		(cd util          && test ! -e Makefile || $(MAKE) clean)
		(cd Add           && test ! -e Makefile || $(MAKE) clean)
		(cd AddCube       && test ! -e Makefile || $(MAKE) clean)
		(cd AddMem        && test ! -e Makefile || $(MAKE) clean)
		(cd ArchiveList   && test ! -e Makefile || $(MAKE) clean)
		(cd ArchiveGet    && test ! -e Makefile || $(MAKE) clean)
		(cd ArchiveExec   && test ! -e Makefile || $(MAKE) clean)
		(cd Background    && test ! -e Makefile || $(MAKE) clean)
		(cd BestImage     && test ! -e Makefile || $(MAKE) clean)
		(cd BgModel       && test ! -e Makefile || $(MAKE) clean)
		(cd Coord         && test ! -e Makefile || $(MAKE) clean)
		(cd CoverageCheck && test ! -e Makefile || $(MAKE) clean)
		(cd Diff          && test ! -e Makefile || $(MAKE) clean)
		(cd DiffFit       && test ! -e Makefile || $(MAKE) clean)
		(cd Examine       && test ! -e Makefile || $(MAKE) clean)
		(cd Fitplane      && test ! -e Makefile || $(MAKE) clean)
		(cd FixNaN        && test ! -e Makefile || $(MAKE) clean)
		(cd Flatten       && test ! -e Makefile || $(MAKE) clean)
		(cd GetHdr        && test ! -e Makefile || $(MAKE) clean)
		(cd Hdr           && test ! -e Makefile || $(MAKE) clean)
		(cd Histogram     && test ! -e Makefile || $(MAKE) clean)
		(cd Imgtbl        && test ! -e Makefile || $(MAKE) clean)
		(cd MakeHdr       && test ! -e Makefile || $(MAKE) clean)
		(cd MakeImg       && test ! -e Makefile || $(MAKE) clean)
		(cd Mask          && test ! -e Makefile || $(MAKE) clean)
		(cd Overlaps      && test ! -e Makefile || $(MAKE) clean)
		(cd ProjectCube   && test ! -e Makefile || $(MAKE) clean)
		(cd Project       && test ! -e Makefile || $(MAKE) clean)
		(cd ProjectPP     && test ! -e Makefile || $(MAKE) clean)
		(cd ProjectQL     && test ! -e Makefile || $(MAKE) clean)
		(cd PutHdr        && test ! -e Makefile || $(MAKE) clean)
		(cd ShrinkCube    && test ! -e Makefile || $(MAKE) clean)
		(cd Shrink        && test ! -e Makefile || $(MAKE) clean)
		(cd SubCube       && test ! -e Makefile || $(MAKE) clean)
		(cd SubHdr        && test ! -e Makefile || $(MAKE) clean)
		(cd Subimage      && test ! -e Makefile || $(MAKE) clean)
		(cd Subset        && test ! -e Makefile || $(MAKE) clean)
		(cd TANHdr        && test ! -e Makefile || $(MAKE) clean)
		(cd TileImage     && test ! -e Makefile || $(MAKE) clean)
		(cd Transpose     && test ! -e Makefile || $(MAKE) clean)
		(cd WWT           && test ! -e Makefile || $(MAKE) clean)
		(cd Viewer        && test ! -e Makefile || $(MAKE) clean)
		(cd DiffExec      && test ! -e Makefile || $(MAKE) clean)
		(cd BgExec        && test ! -e Makefile || $(MAKE) clean)
		(cd DiffFitExec   && test ! -e Makefile || $(MAKE) clean)
		(cd FitExec       && test ! -e Makefile || $(MAKE) clean)
		(cd ProjExec      && test ! -e Makefile || $(MAKE) clean)
		rm -f libmontage.a libmontage.so
		rm -f ../python/MontagePy/lib/*.o
