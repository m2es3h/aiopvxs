import asyncio

from aiopvxs.client import Context

async def main():
    client = Context()
    # start the server discovery with active ping
    discover_op =  client.discover(do_ping = True)

    try:
        async with asyncio.timeout(5):
            print("\nSearching for 5 seconds (ctrl-C to quit) ...")
            # for every discovered server ...
            async for val in discover_op:
                print("\nSERVER FOUND:", val.server)
                # ... get list of PVs ...
                pv_list = await client.list(val.server)
                # ... and print them
                for pv in pv_list.value.as_list():
                    print("    ", pv)
    except (asyncio.CancelledError, asyncio.TimeoutError):
        pass
    finally:
        discover_op.cancel()

asyncio.run(main())
