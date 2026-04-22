#!/usr/bin/env python
import os
import os.path
import sys

from setuptools import Extension, setup

invoked = os.getcwd()
if os.path.dirname(sys.argv[0]) != "":
    os.chdir(os.path.dirname(sys.argv[0]))

setup(
    name="hammer",
    version="1.1.1",
    author="Riverside Research",
    description="The Hammer parser combinator library",
    ext_modules=[
        Extension(
            "_hammer",
            ["hammer.i"],
            swig_opts=["-DHAMMER_INTERNAL__NO_STDARG_H", "-I../../"],
            define_macros=[("SWIG", None)],
            depends=[
                "allocator.h",
                "glue.h",
                "hammer.h",
                "internal.h",
            ],
            extra_compile_args=["-fPIC", "-std=gnu99"],
            include_dirs=["../../"],
            library_dirs=["../../"],
            libraries=["hammer"],
        )
    ],
    py_modules=["hammer"],
)

os.chdir(invoked)
