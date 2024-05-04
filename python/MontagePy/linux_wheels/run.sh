#!/bin/sh

export CIBW_BUILD='*'
export CIBW_ARCHS='x86_64'
export CIBW_BEFORE_ALL='sh make.sh'
export CIBW_BUILD_FRONTEND='build'
export SED='sed'

echo "CIBW_BUILD>         " "$CIBW_BUILD"
echo "CIBW_ARCHS>         " "$CIBW_ARCHS"
echo "CIBW_BEFORE_ALL>    " "$CIBW_BEFORE_ALL"
echo "CIBW_BUILD_FRONTEND>" "$CIBW_BUILD_FRONTEND"
echo "SED>                " "$SED"

pip install pipx

pipx run cibuildwheel --platform linux 

