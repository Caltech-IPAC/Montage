#!/usr/bin/env python

from setuptools import setup

setup(
    name = 'astroMontage',
    version = '0.1',
    author = 'John Good',
    author_email = 'jcg@ipac.caltech.edu',
    description = 'Wrapper around Montage toolkit for displaying astronomical image/overlay data.',
    license = 'BSD',
    keywords = 'astronomy astronomical image display',
    url = 'https://github.com/Caltech-IPAC/Montage',
    
    packages = ['astroMontage'],
    install_requires = ['tornado'],
    package_data = { 'astroMontage': ['web/*'] }
)
