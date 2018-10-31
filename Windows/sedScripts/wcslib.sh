#!/bin/sh

sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' wcs.h
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' wcslib.h
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' wcs.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' wcslib.c
sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' wcsinit.c
