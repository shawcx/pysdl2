#!/usr/bin/env python3

import sys
import os
import platform
import glob

import setuptools

defines      = [('_REENTRANT',None),('_GNU_SOURCE','1')]
include_dirs = ['/usr/local/include']
library_dirs = ['/usr/local/lib']
libs         = ['SDL2', 'SDL2_image']
source_files = glob.glob('./src/*.c')

setuptools.setup(
    name             = 'SDL2',
    version          = '0.2.1',
    author           = 'Matthew Shaw',
    author_email     = 'mshaw.cx@gmail.com',
    url              = 'https://github.com/shawcx/pysdl2',
    license          = 'MIT',
    description      = 'Python3 bindings for SDL2',
    long_description = open('README.rst').read(),
    ext_modules = [
        setuptools.Extension(
            'SDL2',
            source_files,
            define_macros = defines,
            include_dirs  = include_dirs,
            library_dirs  = library_dirs,
            libraries     = libs
            )
        ],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        ]
    )
