import os

from setuptools import setup
from distutils.extension import Extension
from Cython.Build import cythonize

LIB = os.path.join('../..', 'lib')
MONTAGELIB = os.path.join('../..', 'MontageLib')

objs = []
for obj in os.listdir("lib"):
    objs.append("lib/" + obj)

os.environ['CC'] = 'gcc'
os.environ['CFLAGS'] = ''
os.environ['ARCHFLAGS'] = '-arch x86_64'

extensions = [
    Extension("MontagePy._wrappers", ["_wrappers.pyx"],
        include_dirs = [os.path.join(LIB, 'include'), os.path.join(LIB, 'src', 'bzip2-1.0.6'), MONTAGELIB],
        extra_objects = objs),
    Extension("MontagePy.main", ["main.pyx"],
        include_dirs = [os.path.join(LIB, 'include'), MONTAGELIB])
]

setup(
    name = 'MontagePy',
    version = '1.2.0',
    author = 'John Good',
    author_email = 'jcg@ipac.caltech.edu',
    description = 'Montage toolkit for reprojecting, mosaicking, and displaying astronomical images.',
    long_description=open('README.txt').read(),
    license = 'LICENSE.txt',
    keywords = 'astronomy astronomical image reprojection mosaic visualization',
    url = 'https://github.com/Caltech-IPAC/Montage',
    packages = ['MontagePy'],
    package_data = { 'MontagePy': ['FreeSans.ttf'] },
    ext_modules = cythonize(extensions),
    install_requires = ['requests']
)
