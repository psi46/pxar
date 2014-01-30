# setup.py file
import sys
import os
import shutil

from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

# clean previous build
for root, dirs, files in os.walk(".", topdown=False):
    for name in files:
        if (name.startswith("pxarCore") and not(name.endswith(".pyx") or name.endswith(".pxd"))):
            os.remove(os.path.join(root, name))
    for name in dirs:
        if (name == "build"):
            shutil.rmtree(name)

# build "pxarCore.so" python extension to be added to "PYTHONPATH" afterwards...
setup(
    cmdclass = {'build_ext': build_ext},
    ext_modules = [
    Extension("pxarCore", 
              sources=["pxarCore.pyx", \
                       ],
              include_dirs=["../api"],
              libraries=["pxar"],          # refers to shared pXar core library
              language="c++",
              #extra_compile_args=["-fopenmp", "-O3"],
              #extra_link_args=["-DSOME_DEFINE_OPT", "-L./some/extra/dependency/dir/"]
              extra_link_args=["-L./../../lib/"]
              )
    ]
)           
