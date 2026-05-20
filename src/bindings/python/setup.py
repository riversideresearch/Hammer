#!/usr/bin/env python
import os
import os.path
import sys

from setuptools import Extension, setup

invoked = os.getcwd()
if os.path.dirname(sys.argv[0]) != "":
    os.chdir(os.path.dirname(sys.argv[0]))

with open("../../../VERSION", "r") as version_file:
    version = version_file.read().strip()

setup(
    name="hammer",
    version=version,
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
