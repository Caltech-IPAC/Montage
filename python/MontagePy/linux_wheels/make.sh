#!/bin/sh

# Up-front cleanup (should be unnecessary inside container)

rm -rf src
rm -rf build
rm -rf lib
rm -rf dist
rm -rf MontagePy.egg-info
rm -rf wrappers.pxd
rm -rf _wrappers.pyx
rm -rf main.pyx
rm -rf Montage lib/*

mkdir -p src/MontagePy


# Get and build Montage and copy the files from there
# we need to build the wheels

git clone https://github.com/Caltech-IPAC/Montage.git 

(cd Montage && git checkout develop && make)

cp -r Montage/python/MontagePy/lib lib 

cp Montage/python/MontagePy/pyproject.toml .
cp Montage/python/MontagePy/README.txt .
cp Montage/python/MontagePy/LICENSE.txt .
cp -r Montage/python/MontagePy/templates .
cp Montage/python/MontagePy/MontagePy/archive.py src/Montage
cp Montage/python/MontagePy/MontagePy/FreeSans.ttf .


# Build Cython input files for our project

pip install jinja2

python parse.py

$SED '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import importlib_resources\n\n    if fontFile == "":\n        fontFile = str(importlib_resources.files("MontagePy") / "FreeSans.ttf")' _wrappers.pyx > tmpfile

mv tmpfile _wrappers.pyx

