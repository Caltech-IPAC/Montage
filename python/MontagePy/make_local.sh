# /bin/sh

rm -rf wheelhouse dist
rm -rf MontagePy.egg-info build MontagePy/__pycache__
rm -f main.c main.pyx _wrappers.c wrappers.pxd _wrappers.pyx

python parse.py

sed '/^def mViewer/a \ \ \ \ # Next four lines added by sed script\n    import pkg_resources\n\n    if fontFile == "":\n        fontFile = pkg_resources.resource_filename("MontagePy", "FreeSans.ttf")' _wrappers.pyx > tmpfile

mv tmpfile _wrappers.pyx

python setup_manylinux.py build bdist_wheel
