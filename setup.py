from site import getsitepackages, getusersitepackages

import epicscorelibs
import pvxslibs
from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

# get paths to pvxslibs and epicscore libraries DSOs
compiletime_dirs = [*pvxslibs.__path__, *epicscorelibs.__path__]
runtime_dirs = [*getsitepackages(), getusersitepackages(), "@loader_path"]

# declare pybind11 extension
ext_modules = [
    Pybind11Extension(
        name = 'aiopvxs',
        sources = [
            'src/aiopvxs.cpp',
            'src/client.cpp',
            'src/data.cpp',
            'src/nt.cpp',
        ],
        include_dirs=[
            f"{mod_dir}/include" for mod_dir in compiletime_dirs
        ],
        library_dirs=[
            f"{mod_dir}/lib" for mod_dir in compiletime_dirs
        ],
        runtime_library_dirs=[
            *[f"{base_dir}/pvxslibs/lib" for base_dir in runtime_dirs],
            *[f"{base_dir}/epicscorelibs/lib" for base_dir in runtime_dirs],
        ],
        libraries=["pvxs", "event_core", "Com"],
        language='c++',
        cxx_std=11,
    ),
]

setup(
    ext_modules=ext_modules,
)
