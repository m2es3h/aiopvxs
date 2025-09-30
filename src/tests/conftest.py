import pytest
import pytest_asyncio
from typing import AsyncGenerator

from aiopvxs.client import Context
from aiopvxs.data import TypeCodeEnum as T, Value
from aiopvxs.nt import NTScalar
from aiopvxs.server import SharedPV, Server

@pytest_asyncio.fixture()
async def pvxs_test_context() -> AsyncGenerator[Context]:
    yield Context()

@pytest_asyncio.fixture()
async def pvxs_test_server() -> AsyncGenerator[Server]:
    val = NTScalar(T.Int32).create()
    val['value'] = str(-42)
    pv_int32 = SharedPV()
    pv_int32.open(val)

    with Server({"scalar_int32": pv_int32}) as server:
        yield server

    pv_int32.close()
