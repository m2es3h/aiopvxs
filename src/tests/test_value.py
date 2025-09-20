from aiopvxs.data import TypeCodeEnum as T, TypeDef, Member as M
from aiopvxs.nt import NTScalar, NTEnum

"""
x = TypeDef(T.Bool).create()
y0 = M(T.String, "desc")
y1 = M(T.Bool, "flag")
y2 = M(T.Int8, "number32")
y3 = M(T.Int64, "number64")
y4 = M(T.Struct, "substruct", set({y1, y2, y3}))

print(repr(x))

z = TypeDef(T.Struct, [y0, y1, y2, y4])
print(repr(z))


val = z.create()
print("number32 exists ", val.number32)
val["number32"] = 977
print("number32 exists ", val.number32)

val.desc = "some string"
val.flag = True
val.number32 = 988
val.substruct.flag = False
val.substruct.number32 = 32
val.substruct["number64"] = 64
print(repr(val))


nt_val = NTScalar(T.Int32).create();
nt_val = NTEnum().create();
print(repr(nt_val))
"""