#!/bin/sh

sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' checkHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' get_hdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' get_hhdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mAdd.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mAddCube.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mCoverageCheck.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mHdrCheck.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mMakeHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mMakeImg.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mOverlaps.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mProject.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mProjectCube.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mProjectPP.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mSubset.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mTANHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' mTileHdr.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' projTest.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' subCube.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' subImage.c
