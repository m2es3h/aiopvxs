import sys
from pathlib import Path
from site import getsitepackages, getusersitepackages

import epicscorelibs
import pvxslibs
from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

# get paths to pvxslibs and epicscore libraries DSOs
compiletime_dirs = [*pvxslibs.__path__, *epicscorelibs.__path__]
runtime_dirs = [*getsitepackages(), getusersitepackages(), "@loader_path"] if sys.platform != "win32" else []
extra_compile_args=['-D_GLIBCXX_USE_CXX11_ABI=0'] if sys.platform.startswith("linux") else []

# declare pybind11 extension
ext_modules = [
    Pybind11Extension(
        name = 'aiopvxs',
        sources = [
            'src/aiopvxs.cpp',
            'src/client.cpp',
            'src/data.cpp',
            'src/nt.cpp',
            'src/server.cpp',
        ],
        extra_compile_args=extra_compile_args,
        include_dirs=[
            *[str(Path(mod_dir) / "include") for mod_dir in compiletime_dirs],
            # path to this project's src directory
            Path(__file__).parent.resolve() / 'src',
        ],
        library_dirs=[
            str(Path(mod_dir) / "lib") for mod_dir in compiletime_dirs
        ],
        runtime_library_dirs=[
            *[str(Path(base_dir) / "pvxslibs" / "lib") for base_dir in runtime_dirs],
            *[str(Path(base_dir) / "epicscorelibs" / "lib") for base_dir in runtime_dirs],
        ],
        libraries=["pvxs", "event_core", "Com"],
        language='c++',
        cxx_std=11,
    ),
]

setup(
    ext_modules=ext_modules,
    # include files specified in MANIFEST.in
    include_package_data=True
)
