#!/bin/bash

DOCKCROSS_IMAGE=thewtex/cross-compiler-windows-x64

docker run -i -t \
   -v $PWD:/build \
   $DOCKCROSS_IMAGE bash
