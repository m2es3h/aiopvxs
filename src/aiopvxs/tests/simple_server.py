import asyncio

from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.nt import NTScalar
from aiopvxs.server import Server, SharedPV

def callback(pv, op, value):
    print(f"onPUT handler called with new value: {value.value}")
    pv.post(value)
    op.reply()

# create SharedPV with Value
pv_int32 = SharedPV(
    nt=NTScalar(T.Int32A).build(),
    initial={
        'value': [0, -1, -2, -3, -4, -5],
        'alarm.message': "ints are negative"
    }
)
pv_int32.onPut(callback)

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
