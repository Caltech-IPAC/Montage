#!/bin/sh

./configureDocker.sh

(cd lib; make)

(cd /build/lib/src/wcssubs3.9.0_montage; /build/Windows/sedScripts/wcslib.sh)
(cd /build/lib/src/two_plane_v1.1;       /build/Windows/sedScripts/twoplane.sh)
(cd /build/MontageLib;                   /build/Windows/sedScripts/MontageLib.sh)

(cd /build/lib/src;      make -f /build/Windows/Makefiles/Makefile.lib)
(cd /build/MontageLib;   make -f /build/Windows/Makefiles/Makefile.MontageLib)
