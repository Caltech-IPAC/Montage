#!/bin/sh

# This script is for our local use but should work on most 
# systems and is easily modified.

rm -rf src
rm -rf build
rm -rf dist
rm -rf MontagePy.egg-info

mkdir -p src/MontagePy
mkdir -p src/MontagePy/archive

cp __init__.py src/MontagePy
cp __archive__.py src/MontagePy/archive/__init__.py
cp mArchiveList.py mArchiveDownload.py src/MontagePy/archive/
cp ../../data/fonts/FreeSans.ttf src/MontagePy

python parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import importlib_resources\n\n    if fontFile == "":\n        fontFile = str(importlib_resources.files("MontagePy") / "FreeSans.ttf")' src/MontagePy/_wrappers.pyx > src/MontagePy/tmpfile

mv src/MontagePy/tmpfile src/MontagePy/_wrappers.pyx

cp src/MontagePy/wrappers.pxd .

python -m build --wheel
