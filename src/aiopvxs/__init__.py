import os
import sys
from pathlib import Path

import epicscorelibs
import pvxslibs

if sys.platform == "win32":
    runtime_dirs = [*pvxslibs.__path__, *epicscorelibs.__path__]

    for base_dir in runtime_dirs:
        os.add_dll_directory(str(Path(base_dir).resolve() / "lib"))

from aiopvxs._aiopvxs import *
