#!/bin/sh

# This script is for building the Python 2.7/3.6 versions of
# the MontagePy wheels for Mac OSX.  It assumes you have already
# installed 'python2' and 'python3' and have 'Cython', 'jinja2'
# and 'wheel' installed as Python packages in both.


# Python 2.7

rm -rf build dist MontagePy.egg-info MontagePy/__pycache__

python2 parse.py

sed '/^def mViewer/a \
\ \ \ \ # Next four lines added by sed script \
\ \ \ \ import pkg_resources \
\ \ \ \ if fontFile == "": \
\ \ \ \ \ \ \ \ fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf") \
\ \ \ \ \ \ \ \ ' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python2 setup_osx.py build bdist_wheel


# Python 3.6

rm -rf build MontagePy.egg-info MontagePy/__pycache__

python3 parse.py

sed '/^def mViewer/a \
\ \ \ \ # Next four lines added by sed script \
\ \ \ \ import pkg_resources \
\ \ \ \ if fontFile == "": \
\ \ \ \ \ \ \ \ fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf") \
\ \ \ \ \ \ \ \ ' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python3 setup_osx.py build bdist_wheel
