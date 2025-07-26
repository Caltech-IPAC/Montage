#!/bin/sh

echo ''
echo 'CURRENT DIRECTORY'
pwd
ls

mkdir -p src/MontagePy
mkdir -p src/MontagePy/archive
mkdir -p lib

rm -rf /project/Montage


# Get and build Montage

echo ''
echo 'CLONE MONTAGE'
git clone https://github.com/Caltech-IPAC/Montage.git 

echo ''
echo 'MAKE MONTAGE'
(cd Montage && make > make.out)


# Copy the files from the Montage build that we need to build the wheel

cp -r Montage/python/MontagePy/lib/* lib 

cp -r Montage/python/MontagePy/templates      .   
cp    Montage/python/MontagePy/pyproject.toml .
cp    Montage/python/MontagePy/README.md      .   
cp    Montage/python/MontagePy/LICENSE        .   
cp    Montage/python/MontagePy/cleanup.py     .   

cp    Montage/python/MontagePy/__init__.py          src/MontagePy
cp    Montage/python/MontagePy/__archive__.py       src/MontagePy/archive/__init__.py
cp    Montage/python/MontagePy/mArchiveList.py      src/MontagePy/archive
cp    Montage/python/MontagePy/mArchiveDownload.py  src/MontagePy/archive
cp    Montage/data/fonts/FreeSans.ttf               src/MontagePy


# Build Cython input files for our project

pip install jinja2

python parse.py
python cleanup.py src/MontagePy/_wrappers.pyx > tmpfile

mv tmpfile src/MontagePy/_wrappers.pyx

cp src/MontagePy/wrappers.pxd .
