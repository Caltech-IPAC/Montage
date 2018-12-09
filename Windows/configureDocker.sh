#!/bin/sh

mv /usr/bin/gcc /usr/bin/gcc-linux
ln -s /usr/src/mxe/usr/bin/x86_64-w64-mingw32.static-gcc /usr/bin/gcc
