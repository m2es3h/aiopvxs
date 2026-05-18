import asyncio

from aiopvxs.client import Context, RemoteError

# instantiate new client Context
client_ctx = Context()

async def put_forever():
    i = 0
    while True:
        i += 1
        # put new value
        new_value = {
            'value': [1*i, 2*i, 3*i, 4*i, 5*i],
            'alarm.message': "value from put_forever()",
        }
        print("worker task put", new_value['value'])
        await client_ctx.put("test:pv:int32", new_value)
        await asyncio.sleep(0.5)

async def main():
    # get new value
    sub = client_ctx.monitor("test:pv:int32")

    t = asyncio.create_task(put_forever())
    await asyncio.sleep(3)

    async for item in sub:
        print("-----")
        print(item)
        print("*****")
        if item.value[0] > 50:
            break

    sub.cancel()

asyncio.run(main())
