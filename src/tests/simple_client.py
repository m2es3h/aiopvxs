import asyncio

from aiopvxs.client import Context, RemoteError

# instantiate new client Context
client_ctx = Context()

async def main():
    put_op = client_ctx.put("test:pv:int32", {'value': [3]})
    assert isinstance(put_op, asyncio.Future)

    try:
        # call put_op.cancel() before the await to cancel the operation
        await asyncio.wait_for(put_op, timeout=3.0)
    except asyncio.TimeoutError:
        print("put operation failed: Timed out")
    except asyncio.CancelledError:
        print("put operation failed: Operation cancelled")
    except RemoteError as e:
        print("put operation failed: Server returned exception:", e)
    except (KeyError, TypeError, LookupError) as e:
        print("put value not compatible with pvxs.Value type:", e)
    else:
        print("put operation successful")
    finally:
        # asyncio.Future.done() is true in all cases
        assert put_op.done()
            
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
