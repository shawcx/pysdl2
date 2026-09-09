#!/usr/bin/env python3

import glob
import shlex
import subprocess

import setuptools

defines = [('_REENTRANT', None), ('_GNU_SOURCE', '1')]
libs = ['SDL2', 'SDL2_image']
source_files = glob.glob('./src/*.c')


def _probe_sdl():
    '''Ask sdl2-config / pkg-config where SDL2 lives; fall back to the common
    Homebrew and /usr/local prefixes so a bare `pip install .` still works.'''
    include_dirs = []
    library_dirs = []
    for cmd in (['sdl2-config', '--cflags', '--libs'],
                ['pkg-config', '--cflags', '--libs', 'sdl2']):
        try:
            out = subprocess.check_output(cmd, text=True)
        except (OSError, subprocess.CalledProcessError):
            continue
        for token in shlex.split(out):
            if token.startswith('-I'):
                include_dirs.append(token[2:])
            elif token.startswith('-L'):
                library_dirs.append(token[2:])
        break
    include_dirs += ['/usr/local/include', '/opt/homebrew/include']
    library_dirs += ['/usr/local/lib', '/opt/homebrew/lib']
    return include_dirs, library_dirs


include_dirs, library_dirs = _probe_sdl()

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
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Programming Language :: Python :: 3',
        ]
    )
