#!/bin/sh

# This script is specific to building the 'manylinux' wheels
# on a Centos05 platform (in our case under Docker).

rm -rf wheelhouse

for VERSION in $(ls /opt/python); do

   rm -rf src
   rm -rf dist
   rm -rf build
   rm -rf MontagePy.egg-info
   rm -rf wrappers.pxd

   mkdir -p src/MontagePy
   cp ../../data/fonts/FreeSans.ttf src/MontagePy

   /opt/python/$VERSION/bin/python parse.py

   sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import importlib_resources\n\n    if fontFile == "":\n        fontFile = str(importlib_resources.files("MontagePy") / "FreeSans.ttf")' src/MontagePy/_wrappers.pyx > src/MontagePy/tmpfile

   mv src/MontagePy/tmpfile src/MontagePy/_wrappers.pyx

   cp src/MontagePy/wrappers.pxd .

   /opt/python/$VERSION/bin/python -m build --wheel

   auditwheel repair dist/*.whl

done



# Get the host OS file permissions.  The host directory
# where we started Docker is mounted inside the container
# as '/external'.

LSLINE=`ls -l / | grep external`

LSWORDS=($LSLINE)

HOST_UID=${LSWORDS[2]}
HOST_GID=${LSWORDS[3]}



# Copy the wheel files to the host and set ownership.

rm -rf /external/wheelhouse
mkdir  /external/wheelhouse

chown $HOST_UID /external/wheelhouse
chgrp $HOST_GID /external/wheelhouse


for WHEEL in $(ls wheelhouse); do

   cp wheelhouse/$WHEEL /external/wheelhouse/$WHEEL

   chown $HOST_UID /external/wheelhouse/$WHEEL
   chgrp $HOST_GID /external/wheelhouse/$WHEEL

done
