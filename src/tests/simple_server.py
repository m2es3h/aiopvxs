import asyncio

from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.nt import NTScalar
from aiopvxs.server import SharedPV, Server

# create Value
val = NTScalar(T.Int32A).create()
val['value'] = [-1, -2, -3, -4, -5]
val.alarm.message = "ints are negative"
# create SharedPV with Value
pv_int32 = SharedPV()
pv_int32.open(val)

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
