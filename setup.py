import sys
from pathlib import Path
from site import getsitepackages, getusersitepackages

import epicscorelibs
import pvxslibs
from pybind11.setup_helpers import Pybind11Extension
from setuptools import find_namespace_packages, setup

# get paths to pvxslibs and epicscore libraries DSOs
compiletime_dirs = [*pvxslibs.__path__, *epicscorelibs.__path__]
extra_compile_args = ['-D_GLIBCXX_USE_CXX11_ABI=0'] if sys.platform.startswith("linux") else []

if sys.platform == "win32":
    runtime_dirs = []  # os.add_dll_directory() in __init__.py used for windows
elif sys.platform == "darwin":
    runtime_dirs = ["@loader_path/../", getusersitepackages(), *getsitepackages()]
else:
    runtime_dirs = ["$ORIGIN/../", getusersitepackages(), *getsitepackages()]

# declare pybind11 extension
ext_modules = [
    Pybind11Extension(
        name = 'aiopvxs._aiopvxs',
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
    # include pybind11 extension
    ext_modules=ext_modules,
    # include __init__.py loader and unit tests
    package_dir={'': 'src'},
    packages=find_namespace_packages(where='src'),
    # include MANIFEST.in files in source distribution
    include_package_data=True
)
