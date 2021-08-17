#!/bin/bash

DOCKCROSS_IMAGE=docker.io/dockcross/manylinux1-x64

docker run -i -t \
   -v $PWD/..:/build \
   $DOCKCROSS_IMAGE bash
