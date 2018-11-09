#!/bin/sh

sed -i.bak -e 's/wcsinit/wcsInit/' -e 's/wcsset/wcsSet/' -e 's/wcsrev/wcsRev/' two_plane.c
