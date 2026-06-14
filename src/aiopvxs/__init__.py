import os
from pathlib import Path

import epicscorelibs
import pvxslibs

if os.name == "nt":
    runtime_dll_dirs = [*pvxslibs.__path__, *epicscorelibs.__path__]

    for base_dir in runtime_dll_dirs:
        os.add_dll_directory(str(Path(base_dir).resolve() / "lib"))
    del base_dir

from ._aiopvxs import client, data, nt, server

__all__ = ["client", "data", "nt", "server"]
