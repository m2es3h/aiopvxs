import os
from pathlib import Path

import epicscorelibs
import pvxslibs

if os.name == "nt":
    runtime_dll_dirs = [*pvxslibs.__path__, *epicscorelibs.__path__]

    for base_dir in runtime_dll_dirs:
        os.add_dll_directory(str(Path(base_dir).resolve() / "lib"))
    del base_dir

from ._aiopvxs import (client, data, nt, pvxs_version, pvxs_version_abi,
                       pvxs_version_int, server)
from ._version import __version__

__all__ = ["client", "data", "nt", "server", "pvxs_version",
           "pvxs_version_int", "pvxs_version_abi", "__version__"]
