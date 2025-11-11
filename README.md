# aiopvxs
Asynchronous PVAccess client/server API using Python asyncio and pybind11

## Key Features
- Uses pybind11 to generate python bindings to pvxslibs v1.4 C++ library
- Supports PVAccess StaticSource server
- Supports PVAccess get and put client Context operations via python asyncio
- Supports getting/setting pvxs.Value fields with python data types

## aiopvxs Installation

It is recommended to install this project to a python virtual environment from
a cloned copy of this source. To do this manually:
```
git clone https://github.com/m2es3h/aiopvxs.git
cd aiopvxs
python3 -m venv .venv
source .venv/bin/activate
pip install ".[test]"      # or pip install -e ".[test]" for editable mode
pytest -vs .
```

If developing in **Visual Studio Code** (VSCode), follow their instructions on
how to automatically setup a python virtual environment (https://code.visualstudio.com/docs/python/environments).

VSCode's Python extension can be used to discover and run the pytest unit
tests (http://code.visualstudio.com/docs/python/testing).

## Getting Started

aiopvxs provides Python bindings to the pvxslibs C++ library (https://epics-base.github.io/pvxs/).
This module enables asynchronous interaction with PVAccess servers and clients from Python.
Bindings, type casting, and object lifetime management between Python <-> C++ is handled by pybind11.

To test that the module is able to find and load pvxs:

```
python3
>>> import aiopvxs
>>> aiopvxs.get_version_str()
'PVXS 1.4.0 (pip)'
>>>
```

### Simple Server

aiopvxs equivalent example to the pvxs shortest server: https://epics-base.github.io/pvxs/example.html#shortest-server

```
import asyncio

from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.nt import NTScalar
from aiopvxs.server import SharedPV, Server

# create Value
val = NTScalar(T.Int32A).create()
val['value'] = [-1, -2, -3, -4, -5]
val.alarm.message = "ints are negative"
# create SharedPV with Value
pv_int32 = SharedPV()
pv_int32.open(val)

async def main():
    try:
        # run PVAccess server
        with Server({"test:pv:int32": pv_int32}) as srv:
            print("Starting server", srv)
            while True:
                await asyncio.sleep(1)
    except asyncio.CancelledError:
        print("Stopping server")

asyncio.run(main())

# instead of asyncio.run(), could just call Server.run()
# srv = Server({"test:pv:int32": pv_int32})
# srv.run()
```

### Simple Client

aiopvxs equivalent example to the pvxs shortest client: https://epics-base.github.io/pvxs/example.html#client-demo

```
import asyncio

from aiopvxs.client import Context

# instantiate new client Context
client_ctx = Context()

async def main():
    # put new value
    new_value = {
        'value': [1, 2, 3, 4, 5],
        'alarm.message': "ints are positive",
    }
    await client_ctx.put("test:pv:int32", new_value)

    # get new value
    pv_int32 = await client_ctx.get("test:pv:int32")

    # print it
    print("----------- full pvxs.Value structure -----------")
    print(pv_int32)
    print("------ outer-most fields within pvxs.Value ------")
    # or iterate over outer-most members of received value
    for field, contents in pv_int32.as_dict().items():
        print(f"{field} is {contents}")

asyncio.run(main())
```

client.Context operations wrap a pvxs::client::Operation() in an
asyncio.Future() and returns the Future to Python. This asynchronous
operation handle can then be treated like any other asyncio.Future, enabling
the use of all Python's asyncio features.

```
get_value = None
get_op = client_ctx.get("test:pv:int32")
assert isinstance(get_op, asyncio.Future)
try:
    # call get_op.cancel() before the await to cancel the operation
    get_value = await asyncio.wait_for(get_op, timeout=3.0)
except asyncio.TimeoutError:
    print("Cannot complete get operation: Timed out")
except asyncio.CancelledError:
    print("Cannot complete get operation: Operation cancelled")
else:
    print(f"Received {get_value.id()} from get operation")
    if 'value' in get_value.as_dict().keys():
        print(f"value is {get_value.value}")
```
