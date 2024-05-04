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
mkdir -p src/MontagePy/archive


# Get and build Montage and copy the files from the Montage build
# that we need to build the wheels

git clone https://github.com/Caltech-IPAC/Montage.git 

(cd Montage && make)

cp -r Montage/python/MontagePy/lib lib 

cp    Montage/python/MontagePy/pyproject.toml .
cp    Montage/python/MontagePy/LICENSE.txt    .
cp    Montage/python/MontagePy/cleanup.py     .
cp -r Montage/python/MontagePy/templates      .

cp    Montage/python/MontagePy/__init__.py          src/MontagePy
cp    Montage/python/MontagePy/__archive__.py       src/MontagePy/archive/__init__.py
cp    Montage/python/MontagePy/mArchiveList.py      src/MontagePy/archive
cp    Montage/python/MontagePy/mArchiveDownload.py  src/MontagePy/archive
cp    Montage/python/MontagePy/FreeSans.ttf         src/MontagePy


# Build Cython input files for our project

pip install jinja2

python parse.py

python cleanup.py src/MontagePy/_wrappers.pyx > tmpfile

mv tmpfile src/MontagePy/_wrappers.pyx

