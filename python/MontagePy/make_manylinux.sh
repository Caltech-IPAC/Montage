#!/bin/sh

# This script is specifit to building the 'manylinux' wheels
# on a Centos05 platform (in our case under Docker).  Multiple
# Python versions and the auditwheel utility (which we need to
# to touch up wheel files for use on all "manylinux" hosts.
#
# The Docker image we are using is
#
#    pytorch/manylinux-cuda102 
#
# (though this changes rapidly as python version come and go).
#
# In this Docker container, several Python versions (e.g., 3.11)
# are nstalled in, e.g.:
#
#    /opt/python/cp311-cp311/bin
#
# All these Python instances have 'python' and 'pip'.


rm -rf wheelhouse

for VERSION in $(ls /opt/python); do

   rm -rf src
   rm -rf build
   rm -rf dist
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

   rm dist/*

done

mv wheelhouse/* dist
