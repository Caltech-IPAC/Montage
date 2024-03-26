import os

from setuptools import setup
from distutils.extension import Extension
from Cython.Build import cythonize

TOP        = os.path.abspath(os.path.join(os.getcwd(), '../..'))

LIB        = os.path.join(TOP, 'lib')
MONTAGELIB = os.path.join(TOP, 'MontageLib')

objs = []
for obj in os.listdir('lib'):
    objs.append('lib/' + obj)

os.environ['CC'       ] = 'gcc'
os.environ['CFLAGS'   ] = ''
os.environ['ARCHFLAGS'] = '-arch x86_64'

extensions = [
    Extension('MontagePy._wrappers', ['src/MontagePy/_wrappers.pyx'],
        include_dirs = [os.path.join(LIB, 'include'),
                        os.path.join(LIB, 'src', 'bzip2-1.0.6'),
                        MONTAGELIB, 'src/MontagePy'],
        extra_objects = objs),

    Extension('MontagePy.main', ['src/MontagePy/main.pyx'],
        include_dirs = [os.path.join(LIB, 'include'),
                        MONTAGELIB, 'src/MontagePy'])
]

setup(
    packages         = ['src/MontagePy'],

    package_data     = {'MontagePy': ['FreeSans.ttf']},

    ext_modules      = cythonize(extensions, 
                                 compiler_directives={'language_level' : '3str'}),
)
