#!/bin/bash

DOCKCROSS_IMAGE=quay.io/pypa/manylinux1_x86_64

docker run -i -t \
   -v $PWD:/build \
   $DOCKCROSS_IMAGE bash
