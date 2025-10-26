import logging
from asyncio import (CancelledError, all_tasks, create_task, current_task,
                     gather, sleep, wait_for)
from typing import AsyncGenerator

import pytest
import pytest_asyncio
from pytest import FixtureRequest

from aiopvxs.client import Context
from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.data import Value
from aiopvxs.nt import NTScalar
from aiopvxs.server import Server

_log = logging.getLogger(__file__)


@pytest.mark.asyncio
class TestClient:

    #@pytest.mark.parametrize('server_pvs', [
    #    {"scalar_int32": NTScalar(T.Int32).create()}
    #])
    async def test_get_integers(self, pvxs_test_server : Server,
                       pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        get_op =  client.get("scalar_int32")
        val = await get_op
        assert isinstance(val, Value)
        assert int(val.value) == -42
    
    async def test_get_cancel(self, pvxs_test_context : Context):
        client = pvxs_test_context

        get_op = client.get("nonexistent")
        get_op.cancel()

        with pytest.raises(CancelledError) as exc_info:
            val = await get_op

    async def test_get_await(self, pvxs_test_server : Server,
                       pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        async def other_work(i):
            await sleep(i)
            _log.info("Completed %s", current_task().get_name())

        other_tasks = [
            create_task(other_work(0), name="fasttask"),
            create_task(other_work(0), name="fasttask"),
            create_task(other_work(0), name="fasttask"),
            create_task(other_work(0.1), name="slowtask"),
            create_task(other_work(0.1), name="slowtask"),
            create_task(other_work(0.1), name="slowtask"),
        ]

        get_op =  client.get("scalar_int32")
        val = await get_op
        _log.info("Context.get() returned %s", val)
        remaining_tasks = list(t.get_name() for t in all_tasks() if t != current_task())
        await gather(*other_tasks)
        assert all(t == 'slowtask' for t in remaining_tasks)
