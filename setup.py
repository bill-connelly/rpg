#!/usr/bin/env python3
# encoding: utf-8

from distutils.core import setup, Extension

rpygrating_module = Extension('_rpg', 
		sources = ['rpg/_rpg.c'],
                extra_compile_args = ['-O3'])

setup(name='rpg',
      version='0.1.0',
      packages = ['rpg'],
      description='A drifting grating implimentation',
      ext_modules=[rpygrating_module])