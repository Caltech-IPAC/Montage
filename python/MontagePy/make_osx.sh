#!/bin/sh

# This script is for building the Python 2.7/3.8 versions of
# the MontagePy wheels for Mac OSX.  It assumes you have already
# installed 'python2' and 'python3' and have 'Cython', 'jinja2'
# and 'wheel' installed as Python packages in both.

# If our OSX version has gotten ahead of the version used to
# build Cython, we can get a bunch of version mismatch warning
# messages from the loader.  This can be avoided with the following:

export MACOSX_DEPLOYMENT_TARGET=10.9


# Python 2.7

rm -rf build dist MontagePy.egg-info MontagePy/__pycache__

python2.7 parse.py

sed '/^def mViewer/a \
\ \ \ \ # Next four lines added by sed script \
\ \ \ \ import pkg_resources \
\ \ \ \ if fontFile == "": \
\ \ \ \ \ \ \ \ fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf") \
\ \ \ \ \ \ \ \ ' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python2.7 setup_osx.py build bdist_wheel


# Python 3.6

rm -rf build MontagePy.egg-info MontagePy/__pycache__

python3.6 parse.py

sed '/^def mViewer/a \
\ \ \ \ # Next four lines added by sed script \
\ \ \ \ import pkg_resources \
\ \ \ \ if fontFile == "": \
\ \ \ \ \ \ \ \ fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf") \
\ \ \ \ \ \ \ \ ' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python3.6 setup_osx.py build bdist_wheel


# Python 3.7

rm -rf build MontagePy.egg-info MontagePy/__pycache__

python3.7 parse.py

sed '/^def mViewer/a \
\ \ \ \ # Next four lines added by sed script \
\ \ \ \ import pkg_resources \
\ \ \ \ if fontFile == "": \
\ \ \ \ \ \ \ \ fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf") \
\ \ \ \ \ \ \ \ ' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python3.7 setup_osx.py build bdist_wheel


# Python 3.8

rm -rf build MontagePy.egg-info MontagePy/__pycache__

python3.8 parse.py

sed '/^def mViewer/a \
\ \ \ \ # Next four lines added by sed script \
\ \ \ \ import pkg_resources \
\ \ \ \ if fontFile == "": \
\ \ \ \ \ \ \ \ fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf") \
\ \ \ \ \ \ \ \ ' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python3.8 setup_osx.py build bdist_wheel
