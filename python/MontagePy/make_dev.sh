#!/bin/sh

# This script is for our local use but is easily modified.
# Specifically, we have set up our own "package index" server
# (using the pypiserver package) for testing purposes and the
# build uploads the binary packages there.  Then we use pip
# with that to install locally.  Not visible are the 
# ~/.pip/pip.conf and ~/.pypirc file contents needed to
# make this work.  Those are specific to our configuration.


# Python 2.7

ssh mosaic rm /Users/jcg/PyPIserver/packages/MontagePy-0.9-cp27-cp27m-linux_x86_64.whl
rm -rf MontagePy.egg-info build dist MontagePy/__pycache__

python2 parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import pkg_resources\n\n    if fontFile == "":\n        fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf")' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python2 setup_dev.py build bdist_wheel upload -r http://mosaic.ipac.caltech.edu:9022

/usr/bin/yes | pip2 uninstall MontagePy
pip2 install -i http://mosaic.ipac.caltech.edu:9022 MontagePy


# Python 3

ssh mosaic rm /Users/jcg/PyPIserver/packages/MontagePy-0.9-cp36-cp36m-linux_x86_64.whl
rm -rf MontagePy.egg-info build dist MontagePy/__pycache__

python3 parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import pkg_resources\n\n    if fontFile == "":\n        fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf")' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

python3 setup_dev.py build bdist_wheel upload -r http://mosaic.ipac.caltech.edu:9022

/usr/bin/yes | pip3 uninstall MontagePy
pip3 install -i http://mosaic.ipac.caltech.edu:9022 MontagePy
