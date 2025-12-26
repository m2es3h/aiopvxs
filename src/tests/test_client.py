import logging
from asyncio import (CancelledError, Future, Queue, all_tasks, create_task,
                     current_task, gather, timeout, sleep, wait_for)

import pytest

from aiopvxs.client import Context, Discovered, Subscription
from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.data import Value
from aiopvxs.server import Server

_log = logging.getLogger(__file__)


@pytest.mark.asyncio
class TestClient:

    async def test_get_value(self, pvxs_test_server : Server,
                       pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        get_op =  client.get("scalar_int32")
        assert isinstance(get_op, Future)
        val = await get_op
        assert isinstance(val, Value)
        assert 'value' in val.as_dict()
        assert 'alarm' in val.as_dict()
        assert 'timeStamp' in val.as_dict()

        assert int(val.value) == -42
    
    async def test_put_value(self, pvxs_test_server : Server,
                       pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        put_op =  client.put("scalar_int32", {'value': 999, 'alarm.message': "OK"})
        assert isinstance(put_op, Future)
        val = await put_op
        assert isinstance(val, Value)
        assert val.type().code == T.Null

        val = await client.get("scalar_int32")
        assert int(val.value) == 999
        assert str(val.alarm.message) == "OK"

    async def test_get_cancel(self, pvxs_test_context : Context):
        client = pvxs_test_context

        get_op = client.get("nonexistent")
        get_op.cancel()

        with pytest.raises(CancelledError) as exc_info:
            val = await get_op

    async def test_put_cancel(self, pvxs_test_context : Context):
        client = pvxs_test_context

        put_op = client.put("nonexistent", {'value': 42})
        put_op.cancel()

        with pytest.raises(CancelledError) as exc_info:
            val = await put_op

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

    async def test_put_await(self, pvxs_test_server : Server,
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

        put_op =  client.put("scalar_int32", {'value': 42})
        val = await put_op
        _log.info("Context.put() returned %s", val)
        remaining_tasks = list(t.get_name() for t in all_tasks() if t != current_task())
        await gather(*other_tasks)
        assert all(t == 'slowtask' for t in remaining_tasks)

@pytest.mark.asyncio
class TestClientCallbacks:

    async def test_discover(self, pvxs_test_server : Server,
                            pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        def cb_function(discovered : Discovered):
            _log.info("In Context.discover() callback function")
            #print(discovered, discovered.event.name)
            #print(discovered.peerVersion, discovered.peer, discovered.proto, discovered.server)
            assert discovered.event.name == "Online"

        discover_op =  client.discover(cb_function, True)
        assert isinstance(discover_op, Future)

        try:
            await wait_for(discover_op, timeout=0.25)
        except TimeoutError:
            discover_op.cancel()

    async def test_monitor_with_cb(self, pvxs_test_server : Server,
                                   pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        def cb_function(value_update):
            _log.info("In Context.monitor() callback function")
            assert int(value_update.value) == -42

        monitor_op = client.monitor("scalar_int32", cb_function)
        assert isinstance(monitor_op, Future)

        try:
            await wait_for(monitor_op, timeout=0.25)
        except TimeoutError:
            monitor_op.cancel()

    async def test_monitor_with_q(self, pvxs_test_server : Server,
                                  pvxs_test_context : Context):
        server = pvxs_test_server
        client = pvxs_test_context

        q = Queue()

        monitor_op = client.monitor("scalar_int32", q.put_nowait)
        assert isinstance(monitor_op, Future)

        item = await wait_for(q.get(), timeout=0.25)
        _log.info("Context.monitor() returned %s", item)
        assert isinstance(item, Value)

        monitor_op.cancel()
        assert q.empty()
