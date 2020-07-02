# /bin/sh

# This script is specifit to building the 'manylinux' wheels
# on a Centos05 platform (in our case under Docker).  Things
# like python2/python3 and auditwheel are already installed
# in the Docker image we ae using.
#
# In the Docker container, Python 2.7 is installed in
#
#    /opt/python/cp27-cp27m/bin
#
# Python 3.6 and 3.7 are installed in
#
#    /opt/python/cp36-cp36m/bin and
#    /opt/python/cp37-cp37m/bin
#
# All have 'python' and 'pip' but only 3.6 has 'auditwheel'.
# To avoid confusion, we'll use full paths.


mkdir final_dist
mkdir final_wheel


# Python 2.7

rm -rf wheelhouse dist

rm -rf MontagePy.egg-info build MontagePy/__pycache__

/opt/python/cp27-cp27m/bin/python parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import pkg_resources\n\n    if fontFile == "":\n        fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf")' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

/opt/python/cp27-cp27m/bin/python setup_manylinux.py build bdist_wheel

/opt/python/cp36-cp36m/bin/auditwheel repair dist/*.whl

cp wheelhouse/* final_wheel
cp dist/* final_dist





# Python 3.6

rm -rf wheelhouse dist

rm -rf MontagePy.egg-info build MontagePy/__pycache__

/opt/python/cp36-cp36m/bin/python parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import pkg_resources\n\n    if fontFile == "":\n        fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf")' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

/opt/python/cp36-cp36m/bin/python setup_manylinux.py build bdist_wheel

/opt/python/cp36-cp36m/bin/auditwheel repair dist/*.whl

cp wheelhouse/* final_wheel
cp dist/* final_dist





# Python 3.7

rm -rf wheelhouse dist

rm -rf MontagePy.egg-info build MontagePy/__pycache__

/opt/python/cp37-cp37m/bin/python parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import pkg_resources\n\n    if fontFile == "":\n        fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf")' MontagePy/_wrappers.pyx > MontagePy/tmpfile
mv MontagePy/tmpfile MontagePy/_wrappers.pyx

/opt/python/cp37-cp37m/bin/python setup_manylinux.py build bdist_wheel

/opt/python/cp36-cp36m/bin/auditwheel repair dist/*.whl

cp wheelhouse/* final_wheel
cp dist/* final_dist



rm -rf wheelhouse dist
rm -rf MontagePy.egg-info build MontagePy/__pycache__
