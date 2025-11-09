import sys
from typing import AsyncGenerator

import pytest
import pytest_asyncio

from aiopvxs.client import Context
from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.nt import NTScalar
from aiopvxs.server import Server, SharedPV


@pytest_asyncio.fixture()
async def pvxs_test_context() -> AsyncGenerator[Context]:
    yield Context()

@pytest_asyncio.fixture()
async def pvxs_test_server() -> AsyncGenerator[Server]:
    val = NTScalar(T.Int32).create()
    val['value'] = -42
    pv_int32 = SharedPV()
    pv_int32.open(val)

    with Server({"scalar_int32": pv_int32}) as server:
        yield server

    pv_int32.close()

@pytest.fixture(scope="module", params=[
    (NTScalar(T.UInt8),  'B', 2**8-1),
    (NTScalar(T.UInt16), 'H', 2**16-1),
    (NTScalar(T.UInt32), 'I', 2**32-1),
    (NTScalar(T.UInt64), 'Q', sys.maxsize),
    (NTScalar(T.Int8),   'b', -2**7),
    (NTScalar(T.Int16),  'h', -2**15),
    (NTScalar(T.Int32),  'i', -2**31),
    (NTScalar(T.Int64),  'q', -sys.maxsize),
], ids=[
    'uint8', 'uint16', 'uint32', 'uint64',
    'int8', 'int16', 'int32', 'int64',
])
def nt_integer_scalars(request : pytest.FixtureRequest):
    return request.param

@pytest.fixture(scope="module", params=[
    (NTScalar(T.Float32), 'f', -42.2411167),
    (NTScalar(T.Float64), 'd', -42.24111000167),
], ids=[
    'float32', 'float64',
])
def nt_float_scalars(request : pytest.FixtureRequest):
    return request.param

@pytest.fixture(scope="module", params=[
    (NTScalar(T.UInt8A),  'B', [2**7]*3),
    (NTScalar(T.UInt16A), 'H', [2**15]*3),
    (NTScalar(T.UInt32A), 'I', [2**31]*3),
    (NTScalar(T.UInt64A), 'Q', [sys.maxsize]*3),
    (NTScalar(T.Int8A),   'b', [-2**7]*3),
    (NTScalar(T.Int16A),  'h', [-2**15]*3),
    (NTScalar(T.Int32A),  'i', [-2**31]*3),
    (NTScalar(T.Int64A),  'q', [-sys.maxsize]*3),
], ids=[
    'uint8', 'uint16', 'uint32', 'uint64',
    'int8', 'int16', 'int32', 'int64',
])
def nt_integer_arrays(request : pytest.FixtureRequest):
    return request.param

@pytest.fixture(scope="module", params=[
    (NTScalar(T.Float32A), 'f', [-42.24111557]*3),
    (NTScalar(T.Float64A), 'd', [-42.24111000167]*3),
], ids=[
    'float32', 'float64',
])
def nt_float_arrays(request : pytest.FixtureRequest):
    return request.param

@pytest.fixture(scope="module", params=[
    {
        'value': {
            'index': 1,
            'choices': ['OFF', 'ON'],
        },
        'display': {
            'description': "sample description",
        },
        'timeStamp': {
            'secondsPastEpoch': 167555999,
            'nanoseconds': 500,
        }
    }
], ids=[
    'dict'
])
def nt_enum_init_dict(request : pytest.FixtureRequest):
    return request.param
