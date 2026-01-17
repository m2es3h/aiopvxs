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

```bash
git clone https://github.com/m2es3h/aiopvxs.git
cd aiopvxs
python3 -m venv .venv
source .venv/bin/activate
pip install ".[test]"      # or pip install -e ".[test]" for editable mode
pytest -vs .
```

If developing in **Visual Studio Code** (VSCode), follow their instructions on
how to automatically setup a python virtual environment
(https://code.visualstudio.com/docs/python/environments).

VSCode's Python extension can be used to discover and run the pytest unit
tests (http://code.visualstudio.com/docs/python/testing).

## Getting Started

aiopvxs provides Python bindings to the pvxslibs C++ library
(https://epics-base.github.io/pvxs/).
This module enables asynchronous interaction with PVAccess servers and clients
from Python. Bindings, type casting, and object lifetime management between
Python <-> C++ is handled by pybind11.

To test that aiopvxs is able to find and load the pip installed pvxs library:

```bash
python3
>>> import aiopvxs
>>> aiopvxs.get_version_str()
'PVXS 1.4.0 (pip)'
>>>
```

### Simple Server

aiopvxs shortest server example (compare to C++ example:
https://epics-base.github.io/pvxs/example.html#shortest-server)

```python
import asyncio

from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.nt import NTScalar
from aiopvxs.server import Server, SharedPV

# create SharedPV with Value
pv_int32 = SharedPV(
    nt=NTScalar(T.Int32A).build(),
    initial={
        'value': [0, -1, -2, -3, -4, -5],
        'alarm.message': "ints are negative"
    }
)

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

aiopvxs shortest client example (compare to C++ example:
https://epics-base.github.io/pvxs/example.html#client-demo)

```python
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
asyncio.Future() and returns the Future to Python, enabling the use of all
asyncio features to retrieve the result or exception.

The asyncio.Future holds a reference to the C++ Operation() instance until
`await` completes or `asyncio.Future.cancel()` is called on the Future.

```python
put_op = client_ctx.put("test:pv:int32", {'value': [1, 2, 3, 4, 5]})
assert isinstance(put_op, asyncio.Future)

try:
    # call put_op.cancel() before the await to cancel the operation
    await asyncio.wait_for(put_op, timeout=3.0)
except asyncio.TimeoutError:
    print("put operation failed: Timed out")
except asyncio.CancelledError:
    print("put operation failed: Operation cancelled")
except aiopvxs.client.RemoteError as e:
    print("put operation failed: Server returned exception:", e)
except (KeyError, TypeError, LookupError) as e:
    print("put value not compatible with pvxs.Value type:", e)
else:
    print("put operation successful")
finally:
    # asyncio.Future.done() is true in all cases
    assert put_op.done()
```

Calling client.Context.monitor() sets up a callback that puts new values and
exceptions into an asyncio.Queue and returns a pvxs::client::Subscription() that
holds a reference to that Queue. You can then use an ``async for`` loop to
iterate over the Subscription object to get value updates as they arrive. Keep the
reference to the Subscription object to keep the subscription alive.

```python
import asyncio

from aiopvxs.client import Context, Subscription

# instantiate new client Context
client_ctx = Context()

async def main():
    # subscribe to changes in scalar_int32 PV
    monitor_sub = client_ctx.monitor("scalar_int32")
    assert isinstance(monitor_op, Subscription)

    # print out value updates until some condition is reached
    async for val in monitor_sub:
        print("Value is", val.value.as_int())
        if (val.value.as_int() < 0):
            break

    # unsubscribe
    monitor_sub.cancel()

asyncio.run(main())
```

### Working with pvxs.Value object

The pvxs::Value object is the API used to exchange data of arbitrary types
between PVAccess clients and servers. Using pybind11's default type casters
plus custom type casters, aiopvxs enables encoding and decoding pvxs::Value
data using Python data types.

```pycon
python3
>>> import array
>>> from aiopvxs.data import Member as M
>>> from aiopvxs.data import TypeCodeEnum as T
>>> from aiopvxs.data import TypeDef

>>> val_container = TypeDef(T.Struct, [
...    M(T.String, "desc"),
...    M(T.Bool, "flag"),
...    M(T.Int16, "number32"),
...    M(T.Int64A, "array64"),
...    M(T.Struct, "substruct", [
...        M(T.Bool, "flag"),
...        M(T.Int16, "number32"),
...        M(T.Int64A, "array64"),
...    ])
... ]).create()

>>> print(val_container)
struct {
    string desc = ""
    bool flag = false
    int16_t number32 = 0
    int64_t[] array64 = {?}[]
    struct {
        bool flag = false
        int64_t[] array64 = {?}[]
        int16_t number32 = 0
    } substruct
}

>>> val_container.desc = "some string"
>>> val_container['flag'] = True
>>> val_container.number32 = 999
>>> val_container['substruct.flag'] = False
>>> val_container.substruct.number32 = -888
>>> val_container.substruct["array64"] = [1, 2, 3, 4, 5]
>>> print(val_container)
struct {
    string desc = "some string"
    bool flag = true
    int16_t number32 = 999
    int64_t[] array64 = {?}[]
    struct {
        int16_t number32 = -888
        int64_t[] array64 = {5}[1, 2, 3, 4, 5]
        bool flag = false
    } substruct
}
```

Each field in the container is also a value type. Iterating over the Value
container iterates over the outer-most fields of that value. The equivalent
Python value of a field can be unwrapped using Python builtins such as
`int(...)`, `str(...)`, `bool(...)`, `float(...)`, or using one of the
`Value.as_type()` methods:

```pycon
>>> from aiopvxs.data import Value
>>> Value.
Value.as_array(        Value.as_int_list(     Value.get(
Value.as_bool(         Value.as_list(         Value.id(
Value.as_dict(         Value.as_string(       Value.mro()
Value.as_float(        Value.as_string_list(  Value.storageType(
Value.as_float_list(   Value.assign(          Value.type(
Value.as_int(          Value.cloneEmpty(
```

Incompatible conversions will raise the underlying aiopvxs.data.NoConvert
exception, or a "Cast not yet implemented" RuntimeError.
