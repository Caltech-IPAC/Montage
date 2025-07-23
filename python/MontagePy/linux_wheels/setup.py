import os
import platform

from setuptools import setup
from distutils.extension import Extension
from Cython.Build import cythonize

machine = platform.machine()

TOP        = os.path.abspath(os.path.join(os.getcwd(), 'Montage'))
INCLUDEDIR = os.path.join(TOP, 'lib/include')
MONTAGELIB = os.path.join(TOP, 'MontageLib')
OBJDIR     = os.path.join(TOP, 'python/MontagePy/lib')

objs = []
for obj in os.listdir(OBJDIR):
    objs.append(os.path.join(OBJDIR, obj))

print('')
print('TOP:       ', TOP)
print('INCLUDEDIR:', INCLUDEDIR)
print('MONTAGELIB:', MONTAGELIB)
print('OBJDIR:    ', OBJDIR)

print('')
print('OBJECTS:')
print(objs)
print('')

os.environ['CC'       ] = 'gcc'
os.environ['CFLAGS'   ] = ''
os.environ['ARCHFLAGS'] = '-arch ' + machine

extensions = [
    Extension('MontagePy._wrappers', ['src/MontagePy/_wrappers.pyx'],
        include_dirs = [INCLUDEDIR, MONTAGELIB],
        extra_objects = objs),

    Extension('MontagePy.main', ['src/MontagePy/main.pyx'])
]

setup(
    packages         = ['src/MontagePy'],

    package_data     = {'MontagePy': ['FreeSans.ttf']},

    ext_modules      = cythonize(extensions, 
                                 compiler_directives={'language_level' : '3str'})
)
