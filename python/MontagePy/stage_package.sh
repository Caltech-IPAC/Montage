#!/bin/sh
set -eu
unset CDPATH
if [ -L "$0" ]; then
    echo "$0: refusing symlink invocation; run the real script path" >&2
    exit 1
fi
cd "$(dirname "$0")"
# A script that sh found through PATH has no directory in $0, so the cd
# above stays where the caller is. Stop unless this script is here.
if [ ! -f ./stage_package.sh ]; then
    echo "$0: cannot find this script's directory, run it by its path" >&2
    exit 1
fi

# Stage the MontagePy package sources for a wheel build in this
# script's directory, from any working directory. parse.py needs
# jinja2.

rm -rf src
rm -rf build

mkdir -p src/MontagePy
mkdir -p src/MontagePy/archive

cp __init__.py                   src/MontagePy
cp __archive__.py                src/MontagePy/archive/__init__.py
cp mArchiveList.py               src/MontagePy/archive/
cp mArchiveDownload.py           src/MontagePy/archive/
cp ../../data/fonts/FreeSans.ttf src/MontagePy

python parse.py

python cleanup.py src/MontagePy/_wrappers.pyx > src/MontagePy/_wrappers.pyx.tmp

mv src/MontagePy/_wrappers.pyx.tmp src/MontagePy/_wrappers.pyx

cp src/MontagePy/wrappers.pxd .
