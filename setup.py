from setuptools import setup, Extension
import pybind11
import pvxslibs
import epicscorelibs

ext_modules = [
    Extension(
        'aiopvxs',
        ['src/aiopvxs.cpp'],
        include_dirs=[
            pybind11.get_include(),
            *[p + "/include" for p in pvxslibs.__path__],
            *[p + "/include" for p in epicscorelibs.__path__],
        ],
        library_dirs=[
            *[p + "/lib" for p in pvxslibs.__path__],
            *[p + "/lib" for p in epicscorelibs.__path__],
        ],
        runtime_library_dirs=[
            *[p + "/lib" for p in pvxslibs.__path__],
            *[p + "/lib" for p in epicscorelibs.__path__],
        ],
        libraries=[
            "pvxs",
            "event_core",
            "event_pthread",
            "Com",
            "pvAccess",
            "pvData"
        ],
        language='c++',
    ),
]

setup(
    name='pyvxs',
    version='0.1.0',
    ext_modules=ext_modules,
    install_requires=[
        "pvxslibs==1.4.0",
    ],
    zip_safe=False,
)
