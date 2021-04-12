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
		(cd AddMem        && $(MAKE) && $(MAKE) install)
		(cd ArchiveList   && $(MAKE) && $(MAKE) install)
		(cd ArchiveGet    && $(MAKE) && $(MAKE) install)
		(cd ArchiveExec   && $(MAKE) && $(MAKE) install)
		(cd Background    && $(MAKE) && $(MAKE) install)
		(cd BestImage     && $(MAKE) && $(MAKE) install)
		(cd BgModel       && $(MAKE) && $(MAKE) install)
		(cd CoverageCheck && $(MAKE) && $(MAKE) install)
		(cd Diff          && $(MAKE) && $(MAKE) install)
		(cd Examine       && $(MAKE) && $(MAKE) install)
		(cd Fitplane      && $(MAKE) && $(MAKE) install)
		(cd FixNaN        && $(MAKE) && $(MAKE) install)
		(cd GetHdr        && $(MAKE) && $(MAKE) install)
		(cd Hdr           && $(MAKE) && $(MAKE) install)
		(cd Histogram     && $(MAKE) && $(MAKE) install)
		(cd Imgtbl        && $(MAKE) && $(MAKE) install)
		(cd MakeHdr       && $(MAKE) && $(MAKE) install)
		(cd MakeImg       && $(MAKE) && $(MAKE) install)
		(cd Overlaps      && $(MAKE) && $(MAKE) install)
		(cd Project       && $(MAKE) && $(MAKE) install)
		(cd ProjectCube   && $(MAKE) && $(MAKE) install)
		(cd ProjectPP     && $(MAKE) && $(MAKE) install)
		(cd ProjectQL     && $(MAKE) && $(MAKE) install)
		(cd PutHdr        && $(MAKE) && $(MAKE) install)
		(cd ShrinkCube    && $(MAKE) && $(MAKE) install)
		(cd Shrink        && $(MAKE) && $(MAKE) install)
		(cd SubCube       && $(MAKE) && $(MAKE) install)
		(cd Subimage      && $(MAKE) && $(MAKE) install)
		(cd SubHdr        && $(MAKE) && $(MAKE) install)
		(cd TANHdr        && $(MAKE) && $(MAKE) install)
		(cd Transpose     && $(MAKE) && $(MAKE) install)
		(cd WWT           && $(MAKE) && $(MAKE) install)
		(cd Viewer        && $(MAKE) && $(MAKE) install)
		(cd DiffExec      && $(MAKE) && $(MAKE) install)
		(cd BgExec        && $(MAKE) && $(MAKE) install)
		(cd DiffFitExec   && $(MAKE) && $(MAKE) install)
		(cd FitExec       && $(MAKE) && $(MAKE) install)
		(cd ProjExec      && $(MAKE) && $(MAKE) install)

lib:
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
			SubHdr/montageSubHdr.o \
			TANHdr/montageTANHdr.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o \
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
			SubHdr/montageSubHdr.o \
			TANHdr/montageTANHdr.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o \
			BgExec/montageBgExec.o \
			DiffExec/montageDiffExec.o \
			FitExec/montageFitExec.o \
			DiffFitExec/montageDiffFitExec.o \
			ProjExec/montageProjExec.o 

pythonlibs:
		rm -f ../python/MontagePy/lib/*.o
		mkdir -p ../python/MontagePy/lib
		cp util/checkFile.o util/checkHdr.o util/checkWCS.o \
			util/debugCheck.o util/filePath.o \
			Add/montageAdd.o \
			AddCube/montageAddCube.o \
			ArchiveList/montageArchiveList.o \
			ArchiveGet/montageArchiveGet.o \
			ArchiveExec/montageArchiveExec.o \
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
			TANHdr/montageTANHdr.o \
			Transpose/montageTranspose.o \
			Viewer/montageViewer.o \
			Viewer/mViewer_graphics.o \
			Viewer/mViewer_grid.o \
			BgExec/montageBgExec.o \
			DiffExec/montageDiffExec.o \
			FitExec/montageFitExec.o \
			DiffFitExec/montageDiffFitExec.o \
			ProjExec/montageProjExec.o \
			../lib/src/boundaries/boundaries.o \
			../lib/src/cfitsio-3.25/buffers.o \
			../lib/src/cfitsio-3.25/cfileio.o \
			../lib/src/cfitsio-3.25/checksum.o \
			../lib/src/cfitsio-3.25/drvrfile.o \
			../lib/src/cfitsio-3.25/drvrgsiftp.o \
			../lib/src/cfitsio-3.25/drvrmem.o \
			../lib/src/cfitsio-3.25/drvrnet.o \
			../lib/src/cfitsio-3.25/drvrsmem.o \
			../lib/src/cfitsio-3.25/editcol.o \
			../lib/src/cfitsio-3.25/edithdu.o \
			../lib/src/cfitsio-3.25/eval_f.o \
			../lib/src/cfitsio-3.25/eval_l.o \
			../lib/src/cfitsio-3.25/eval_y.o \
			../lib/src/cfitsio-3.25/f77_wrap1.o \
			../lib/src/cfitsio-3.25/f77_wrap2.o \
			../lib/src/cfitsio-3.25/f77_wrap3.o \
			../lib/src/cfitsio-3.25/f77_wrap4.o \
			../lib/src/cfitsio-3.25/fitscore.o \
			../lib/src/cfitsio-3.25/fits_hcompress.o \
			../lib/src/cfitsio-3.25/fits_hdecompress.o \
			../lib/src/cfitsio-3.25/getcolb.o \
			../lib/src/cfitsio-3.25/getcold.o \
			../lib/src/cfitsio-3.25/getcole.o \
			../lib/src/cfitsio-3.25/getcoli.o \
			../lib/src/cfitsio-3.25/getcolj.o \
			../lib/src/cfitsio-3.25/getcolk.o \
			../lib/src/cfitsio-3.25/getcoll.o \
			../lib/src/cfitsio-3.25/getcol.o \
			../lib/src/cfitsio-3.25/getcolsb.o \
			../lib/src/cfitsio-3.25/getcols.o \
			../lib/src/cfitsio-3.25/getcolui.o \
			../lib/src/cfitsio-3.25/getcoluj.o \
			../lib/src/cfitsio-3.25/getcoluk.o \
			../lib/src/cfitsio-3.25/getkey.o \
			../lib/src/cfitsio-3.25/group.o \
			../lib/src/cfitsio-3.25/grparser.o \
			../lib/src/cfitsio-3.25/histo.o \
			../lib/src/cfitsio-3.25/imcompress.o \
			../lib/src/cfitsio-3.25/iraffits.o \
			../lib/src/cfitsio-3.25/modkey.o \
			../lib/src/cfitsio-3.25/pliocomp.o \
			../lib/src/cfitsio-3.25/putcolb.o \
			../lib/src/cfitsio-3.25/putcold.o \
			../lib/src/cfitsio-3.25/putcole.o \
			../lib/src/cfitsio-3.25/putcoli.o \
			../lib/src/cfitsio-3.25/putcolj.o \
			../lib/src/cfitsio-3.25/putcolk.o \
			../lib/src/cfitsio-3.25/putcoll.o \
			../lib/src/cfitsio-3.25/putcol.o \
			../lib/src/cfitsio-3.25/putcolsb.o \
			../lib/src/cfitsio-3.25/putcols.o \
			../lib/src/cfitsio-3.25/putcolui.o \
			../lib/src/cfitsio-3.25/putcoluj.o \
			../lib/src/cfitsio-3.25/putcoluk.o \
			../lib/src/cfitsio-3.25/putcolu.o \
			../lib/src/cfitsio-3.25/putkey.o \
			../lib/src/cfitsio-3.25/quantize.o \
			../lib/src/cfitsio-3.25/region.o \
			../lib/src/cfitsio-3.25/ricecomp.o \
			../lib/src/cfitsio-3.25/scalnull.o \
			../lib/src/cfitsio-3.25/swapproc.o \
			../lib/src/cfitsio-3.25/wcssub.o \
			../lib/src/cfitsio-3.25/wcsutil.o \
			../lib/src/cmd/cmd.o \
			../lib/src/coord/ccalc.o \
			../lib/src/coord/computeFKCorrections.o \
			../lib/src/coord/convertBesselianJulian.o \
			../lib/src/coord/convertCoordinates.o \
			../lib/src/coord/convertEclEqu.o \
			../lib/src/coord/convertEquGal.o \
			../lib/src/coord/convertGalSgal.o \
			../lib/src/coord/decimalDegreeToSex.o \
			../lib/src/coord/parseCoordinateString.o \
			../lib/src/coord/precessBesselian.o \
			../lib/src/coord/precessJulian.o \
			../lib/src/coord/sexToDecimalDegree.o \
			../lib/src/jpeg-8b/jaricom.o \
			../lib/src/jpeg-8b/jcapimin.o \
			../lib/src/jpeg-8b/jcapistd.o \
			../lib/src/jpeg-8b/jcarith.o \
			../lib/src/jpeg-8b/jccoefct.o \
			../lib/src/jpeg-8b/jccolor.o \
			../lib/src/jpeg-8b/jcdctmgr.o \
			../lib/src/jpeg-8b/jchuff.o \
			../lib/src/jpeg-8b/jcinit.o \
			../lib/src/jpeg-8b/jcmainct.o \
			../lib/src/jpeg-8b/jcmarker.o \
			../lib/src/jpeg-8b/jcmaster.o \
			../lib/src/jpeg-8b/jcomapi.o \
			../lib/src/jpeg-8b/jcparam.o \
			../lib/src/jpeg-8b/jcprepct.o \
			../lib/src/jpeg-8b/jcsample.o \
			../lib/src/jpeg-8b/jctrans.o \
			../lib/src/jpeg-8b/jdapimin.o \
			../lib/src/jpeg-8b/jdapistd.o \
			../lib/src/jpeg-8b/jdarith.o \
			../lib/src/jpeg-8b/jdatadst.o \
			../lib/src/jpeg-8b/jdatasrc.o \
			../lib/src/jpeg-8b/jdcoefct.o \
			../lib/src/jpeg-8b/jdcolor.o \
			../lib/src/jpeg-8b/jddctmgr.o \
			../lib/src/jpeg-8b/jdhuff.o \
			../lib/src/jpeg-8b/jdinput.o \
			../lib/src/jpeg-8b/jdmainct.o \
			../lib/src/jpeg-8b/jdmarker.o \
			../lib/src/jpeg-8b/jdmaster.o \
			../lib/src/jpeg-8b/jdmerge.o \
			../lib/src/jpeg-8b/jdpostct.o \
			../lib/src/jpeg-8b/jdsample.o \
			../lib/src/jpeg-8b/jdtrans.o \
			../lib/src/jpeg-8b/jerror.o \
			../lib/src/jpeg-8b/jfdctflt.o \
			../lib/src/jpeg-8b/jfdctfst.o \
			../lib/src/jpeg-8b/jfdctint.o \
			../lib/src/jpeg-8b/jidctflt.o \
			../lib/src/jpeg-8b/jidctfst.o \
			../lib/src/jpeg-8b/jidctint.o \
			../lib/src/jpeg-8b/jmemmgr.o \
			../lib/src/jpeg-8b/jmemnobs.o \
			../lib/src/jpeg-8b/jquant1.o \
			../lib/src/jpeg-8b/jquant2.o \
			../lib/src/jpeg-8b/jutils.o \
			../lib/src/json/jsonlib.o \
			../lib/src/lodepng_20140823/lodepng.o \
			../lib/src/mtbl/mtbl.o \
			../lib/src/pixbounds/pixbounds.o \
			../lib/src/two_plane_v1.1/initdistdata.o \
			../lib/src/two_plane_v1.1/redefine_pointing.o \
			../lib/src/two_plane_v1.1/two_plane.o \
			../lib/src/two_plane_v1.1/undistort.o \
			../lib/src/wcssubs3.9.0_montage/cel.o \
			../lib/src/wcssubs3.9.0_montage/dateutil.o \
			../lib/src/wcssubs3.9.0_montage/dsspos.o \
			../lib/src/wcssubs3.9.0_montage/fileutil.o \
			../lib/src/wcssubs3.9.0_montage/fitsfile.o \
			../lib/src/wcssubs3.9.0_montage/hget.o \
			../lib/src/wcssubs3.9.0_montage/hput.o \
			../lib/src/wcssubs3.9.0_montage/iget.o \
			../lib/src/wcssubs3.9.0_montage/imhfile.o \
			../lib/src/wcssubs3.9.0_montage/imio.o \
			../lib/src/wcssubs3.9.0_montage/lin.o \
			../lib/src/wcssubs3.9.0_montage/platepos.o \
			../lib/src/wcssubs3.9.0_montage/poly.o \
			../lib/src/wcssubs3.9.0_montage/proj.o \
			../lib/src/wcssubs3.9.0_montage/sph.o \
			../lib/src/wcssubs3.9.0_montage/tnxpos.o \
			../lib/src/wcssubs3.9.0_montage/wcscon.o \
			../lib/src/wcssubs3.9.0_montage/wcsinit.o \
			../lib/src/wcssubs3.9.0_montage/wcslib.o \
			../lib/src/wcssubs3.9.0_montage/wcs.o \
			../lib/src/wcssubs3.9.0_montage/wcstrig.o \
			../lib/src/wcssubs3.9.0_montage/worldpos.o \
			../lib/src/wcssubs3.9.0_montage/zpxpos.o \
			../lib/src/www/www.o \
			../lib/src/freetype-2.9.1//objs/.libs/autofit.o \
			../lib/src/freetype-2.9.1//objs/.libs/bdf.o \
			../lib/src/freetype-2.9.1//objs/.libs/cff.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftbase.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftbbox.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftbdf.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftbitmap.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftbzip2.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftcache.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftcid.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftdebug.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftfstype.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftgasp.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftglyph.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftgxval.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftgzip.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftinit.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftlzw.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftmm.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftotval.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftpatent.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftpfr.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftstroke.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftsynth.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftsystem.o \
			../lib/src/freetype-2.9.1//objs/.libs/fttype1.o \
			../lib/src/freetype-2.9.1//objs/.libs/ftwinfnt.o \
			../lib/src/freetype-2.9.1//objs/.libs/pcf.o \
			../lib/src/freetype-2.9.1//objs/.libs/pfr.o \
			../lib/src/freetype-2.9.1//objs/.libs/psaux.o \
			../lib/src/freetype-2.9.1//objs/.libs/pshinter.o \
			../lib/src/freetype-2.9.1//objs/.libs/psnames.o \
			../lib/src/freetype-2.9.1//objs/.libs/raster.o \
			../lib/src/freetype-2.9.1//objs/.libs/sfnt.o \
			../lib/src/freetype-2.9.1//objs/.libs/smooth.o \
			../lib/src/freetype-2.9.1//objs/.libs/truetype.o \
			../lib/src/freetype-2.9.1//objs/.libs/type1.o \
			../lib/src/freetype-2.9.1//objs/.libs/type1cid.o \
			../lib/src/freetype-2.9.1//objs/.libs/type42.o \
			../lib/src/freetype-2.9.1//objs/.libs/winfnt.o \
			../lib/src/bzip2-1.0.6/blocksort.o \
			../lib/src/bzip2-1.0.6/huffman.o \
			../lib/src/bzip2-1.0.6/crctable.o \
			../lib/src/bzip2-1.0.6/randtable.o \
			../lib/src/bzip2-1.0.6/decompress.o \
			../lib/src/bzip2-1.0.6/bzlib.o \
         ../python/MontagePy/lib
		cp ../lib/src/two_plane_v1.1/distort.o       ../python/MontagePy/lib/two_plane_distort.o
		cp ../lib/src/wcssubs3.9.0_montage/distort.o ../python/MontagePy/lib/distort.o
		cp ../lib/src/bzip2-1.0.6/compress.o         ../python/MontagePy/lib/bzip2_compress.o
		cp ../lib/src/cfitsio-3.25/compress.o        ../python/MontagePy/lib/compress.o
# The last four lines are needed because we have .o file name conflicts

doc:
			gcc -o mLibDoc mLibDoc.c
			mLibDoc Add
			mLibDoc AddCube
			mLibDoc ArchiveList
			mLibDoc ArchiveGet
			mLibDoc ArchiveExec
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
			mLibDoc TANHdr
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
			mJupyter ArchiveList
			mJupyter ArchiveGet
			mJupyter ArchiveExec
			mJupyter Background
			mJupyter BestImage
			mJupyter BgModel
			mJupyter CoverageCheck
			mJupyter Diff
			mJupyter Examine
			mJupyter Fitplane
			mJupyter FixNaN
			mJupyter GetHdr
			mJupyter Hdr
			mJupyter Histogram
			mJupyter Imgtbl
			mJupyter MakeHdr
			mJupyter MakeImg
			mJupyter Overlaps
			mJupyter ProjectCube
			mJupyter Project
			mJupyter ProjectPP
			mJupyter ProjectQL
			mJupyter PutHdr
			mJupyter ShrinkCube
			mJupyter Shrink
			mJupyter SubCube
			mJupyter Subimage
			mJupyter TANHdr
			mJupyter Transpose
			mJupyter Viewer
			mJupyter BgExec
			mJupyter DiffExec
			mJupyter FitExec
			mJupyter DiffFitExec
			mJupyter ProjExec

clean:
		(cd util          && $(MAKE) clean)
		(cd Add           && test ! -e Makefile || $(MAKE) clean)
		(cd AddCube       && test ! -e Makefile || $(MAKE) clean)
		(cd ArchiveList   && test ! -e Makefile || $(MAKE) clean)
		(cd ArchiveGet    && $(MAKE) clean)
		(cd ArchiveExec   && $(MAKE) clean)
		(cd Background    && test ! -e Makefile || $(MAKE) clean)
		(cd BestImage     && $(MAKE) clean)
		(cd BgModel       && $(MAKE) clean)
		(cd CoverageCheck && test ! -e Makefile || $(MAKE) clean)
		(cd Diff          && test ! -e Makefile || $(MAKE) clean)
		(cd Examine       && test ! -e Makefile || $(MAKE) clean)
		(cd Fitplane      && $(MAKE) clean)
		(cd FixNaN        && test ! -e Makefile || $(MAKE) clean)
		(cd GetHdr        && $(MAKE) clean)
		(cd Hdr           && test ! -e Makefile || $(MAKE) clean)
		(cd Histogram     && $(MAKE) clean)
		(cd Imgtbl        && test ! -e Makefile || $(MAKE) clean)
		(cd MakeHdr       && test ! -e Makefile || $(MAKE) clean)
		(cd MakeImg       && test ! -e Makefile || $(MAKE) clean)
		(cd Overlaps      && test ! -e Makefile || $(MAKE) clean)
		(cd ProjectCube   && test ! -e Makefile || $(MAKE) clean)
		(cd Project       && test ! -e Makefile || $(MAKE) clean)
		(cd ProjectPP     && test ! -e Makefile || $(MAKE) clean)
		(cd ProjectQL     && test ! -e Makefile || $(MAKE) clean)
		(cd PutHdr        && $(MAKE) clean)
		(cd ShrinkCube    && $(MAKE) clean)
		(cd Shrink        && $(MAKE) clean)
		(cd SubCube       && test ! -e Makefile || $(MAKE) clean)
		(cd Subimage      && test ! -e Makefile || $(MAKE) clean)
		(cd TANHdr        && $(MAKE) clean)
		(cd Transpose     && test ! -e Makefile || $(MAKE) clean)
		(cd WWT           && test ! -e Makefile || $(MAKE) clean)
		(cd Viewer        && test ! -e Makefile || $(MAKE) clean)
		(cd DiffExec      && $(MAKE) clean)
		(cd BgExec        && $(MAKE) clean)
		(cd DiffFitExec   && $(MAKE) clean)
		(cd FitExec       && $(MAKE) clean)
		(cd ProjExec      && $(MAKE) clean)
		rm -f libmontage.a libmontage.so
		rm -f ../python/MontagePy/lib/*.o
