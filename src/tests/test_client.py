from unittest.mock import AsyncMock
import logging

from typing import AsyncGenerator
import pytest
import pytest_asyncio
from pytest import FixtureRequest
from aiopvxs.client import Context
from asyncio import wait_for, CancelledError

_log = logging.getLogger(__file__)

@pytest.fixture()
async def pvxs_context() -> AsyncGenerator[Context]:
    yield Context()

@pytest.mark.asyncio
class TestClient:

    async def test_get(self, pvxs_context : AsyncMock):
        client = pvxs_context
        _log.info(pvxs_context)
        get_op =  client.get("some:pv:name")
        val = await get_op
        """
        print("-----")
        print(int(val.get("value")), type(val.value.as_string()))
        print("-----")
        print(repr(val.value))
        print("-----")
        print(str(val.value))
        """
    
    async def test_get_cancel(self, pvxs_context : AsyncMock):
        client = pvxs_context
        _log.info(pvxs_context)
        get_op = client.get("some:pv:name")
        get_op.cancel()

        with pytest.raises(CancelledError) as exc_info:
            val = await get_op
