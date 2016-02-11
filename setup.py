#!/usr/bin/env python3

from distutils.core import setup, Extension

fpsources = ["adafruitfingerprintreader/"+x  for x in "Adafruit_Fingerprint.cpp  fpsensor.cpp  uart.cpp".split()]


module1 = Extension('fpsensor',
                    sources = fpsources,
                    include_dirs = [ "/usr/include/python3.4m/" ],
                    extra_compile_args=["-fno-stack-protector","-g3"]
                    )

setup (version = '1.0',
       description = '',
       ext_modules = [module1])



