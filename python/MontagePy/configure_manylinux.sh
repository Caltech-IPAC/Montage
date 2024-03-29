#!/bin/sh

for VERSION in $(ls /opt/python); do

   /opt/python/$VERSION/bin//bin/python -m pip install --upgrade pip

   /opt/python/$VERSION/bin/pip install build
   /opt/python/$VERSION/bin/pip install Cython
   /opt/python/$VERSION/bin/pip install jinja2

done
