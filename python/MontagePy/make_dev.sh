#!/bin/sh

# This script is for our local use but is easily modified.

rm -rf src/MontagePy/main.pyx
rm -rf src/MontagePy/_wrappers.pyx
rm -rf src/MontagePy/wrappers.pxd
rm -rf wrappers.pxd

rm -rf dist/*

cp setup_manylinux.py setup.py

python parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import importlib_resources\n\n    if fontFile == "":\n        fontFile = str(importlib_resources.files("MontagePy") / "FreeSans.ttf")' src/MontagePy/_wrappers.pyx > src/MontagePy/tmpfile

mv src/MontagePy/tmpfile src/MontagePy/_wrappers.pyx

cp src/MontagePy/wrappers.pxd .

python -m build --wheel
