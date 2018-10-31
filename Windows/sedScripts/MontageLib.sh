#!/bin/sh

sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Add/montageAdd.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' AddCube/montageAddCube.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Background/montageBackground.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' BestImage/montageBestImage.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' CoverageCheck/montageCoverageCheck.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Examine/montageExamine.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' FixNaN/montageFixNaN.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Imgtbl/montageImgtbl.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' MakeHdr/montageMakeHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' MakeImg/montageMakeImg.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Overlaps/montageOverlaps.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' ProjExec/montageProjExec.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Project/montageProject.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' ProjectCube/montageProjectCube.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' ProjectPP/montageProjectPP.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' ProjectQL/montageProjectQL.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' SubCube/montageSubCube.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Subimage/montageSubimage.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Subset/montageSubset.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' TANHdr/montageTANHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' util/checkHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' Viewer/montageViewer.c

