#!/bin/sh

# Before running this script, there are a few things that need to get
# installed if you don't already have them.

# The most important thing is to install the versions of Python we are trying to
# support.  The varies with time as versions appear and others get sunsetted.
# It is best to install the copies from python.org (not, for instance, from Anaconda)
# using the .pkg files the have there.  The cibuildwheel utility will find and use
# each of these in turn (based on the CIBW_BUILD environment variable below).

# These instances of python get installed in /Library/Frameworks/Python.framework/Versions
# and don't interfer with any other copy you might have so you can delete them when you
# are sure you are done using them.

# We might be able to use the local copy of our Montage (outside the container) as the
# basis of the build but we instead pull a new copy into the container and compile it there. 
# If we used the local copy, we would need to compile it with the right OSX deployment
# target and that could get overly confusing.

# The cibuildwheel utility uses environment variables to control what it actually 
# builds.  In addition, on MacOS, the compiler (clang) also checks an environment
# variable to determine the OS version target for the compilation (a necessary concern
# for MacOS builds and OS versions must match if you are linking code together).

# Different settings are used for Linux, so we set things depending on what 'uname'
# says about the system.

export OS='macos'
export CIBW_BUILD='*'
export CIBW_BUILD_FRONTEND='build'
export CIBW_ARCHS='universal2'
export MACOSX_DEPLOYMENT_TARGET='11.1'

echo "OS>                 " "$OS"
echo "CIBW_ARCHS>         " "$CIBW_ARCHS"
echo "CIBW_BUILD>         " "$CIBW_BUILD"
echo "CIBW_BUILD_FRONTEND>" "$CIBW_BUILD_FRONTEND"


# Up-front cleanup

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


# Get and build Montage and copy the files from there
# we need to build the wheels

git clone https://github.com/Caltech-IPAC/Montage.git 

(cd Montage && make)

cp -r Montage/python/MontagePy/lib lib 

cp    Montage/python/MontagePy/pyproject.toml .
cp    Montage/python/MontagePy/README.txt     .
cp    Montage/python/MontagePy/LICENSE.txt    .
cp    Montage/python/MontagePy/cleanup.py     .
cp -r Montage/python/MontagePy/templates      .

cp    Montage/python/MontagePy/MontagePy/__archive__.py   src/Montage/archive/__init__.py
cp    Montage/python/MontagePy/MontagePy/mArchiveList.py  src/Montage/archive
cp    Montage/python/MontagePy/MontagePy/mArchiveDownload src/Montage/archive
cp    Montage/python/MontagePy/MontagePy/FreeSans.ttf     src/MontagePy


# Build Cython input files for our project

pip install jinja2

python parse.py

python cleanup.py src/MontagePy/_wrappers.pyx > src/MontagePy/tmpfile

mv tmpfile _wrappers.pyx


# Build all the MacOS-related wheels

pip install pipx

pipx run cibuildwheel --platform $OS

