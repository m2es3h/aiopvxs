import asyncio

from aiopvxs.client import Context

# instantiate new client Context
client_ctx = Context()

async def main():
    """
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
    """
            
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
