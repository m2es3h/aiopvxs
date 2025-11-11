import array

from aiopvxs.data import Member as M
from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.data import TypeDef
from aiopvxs.nt import NTEnum, NTScalar

y0 = M(T.String, "desc")
y1 = M(T.Bool, "flag")
y2 = M(T.Int16, "number32")
y3 = M(T.Int64A, "array64")
y4 = M(T.Struct, "substruct", set({y1, y2, y3}))

val_container = TypeDef(T.Struct, [y0, y1, y2, y3, y4]).create()
print(val_container)
"""
struct {
    string desc = ""
    bool flag = false
    int16_t number32 = 0
    int64_t[] array64 = {?}[]
    struct {
        bool flag = false
        int64_t[] array64 = {?}[]
        int16_t number32 = 0
    } substruct
}
"""

val_container.desc = "some string"
val_container['flag'] = True
val_container.number32 = 999
val_container.substruct.flag = False
val_container.substruct.number32 = -888
val_container.substruct["array64"] = [1, 2, 3, 4, 5]
print(val_container)
"""
struct {
    string desc = "some string"
    bool flag = true
    int16_t number32 = 999
    int64_t[] array64 = {?}[]
    struct {
        int16_t number32 = -888
        int64_t[] array64 = {5}[1, 2, 3, 4, 5]
        bool flag = false
    } substruct
}
"""

print(int(val_container.number32), str(val_container.number32))
"""
999 '999'
"""
print(val_container.substruct.as_dict())
"""
{'flag': False, 'array64': [1, 2, 3, 4, 5], 'number32': -888}
"""
print([repr(val) for val in val_container.substruct])
"""
['int64_t[] = {5}[1, 2, 3, 4, 5]\n', 'int16_t = -888\n', 'bool = false\n']
"""
val_array = val_container.substruct.array64.as_array()
print(type(val_array), val_array)
"""
<class 'array.array'> array('q', [1, 2, 3, 4, 5])
"""
