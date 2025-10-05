import pytest
import logging
import sys

from aiopvxs.data import TypeCodeEnum as T, TypeDef, Member as M, Value
from aiopvxs.nt import NTScalar, NTEnum

_log = logging.getLogger(__file__)


class TestScalar:

    @pytest.mark.parametrize('nt_numeric_scalars', [
        (NTScalar(T.UInt8), 2**8-1),
        (NTScalar(T.UInt16), 2**16-1),
        (NTScalar(T.UInt32), 2**32-1),
        (NTScalar(T.UInt64), sys.maxsize),
        (NTScalar(T.Int8), -2**7),
        (NTScalar(T.Int16), -2**15),
        (NTScalar(T.Int32), -2**31),
        (NTScalar(T.Int64), -sys.maxsize),
        (NTScalar(T.Float32), -42.2411167),
        (NTScalar(T.Float64), -42.24111000167),
    ], ids=[
        'uint8',
        'uint16',
        'uint32',
        'uint64',
        'int8',
        'int16',
        'int32',
        'int64',
        'float32',
        'float64',
    ], indirect=False)
    def test_wrap_numeric_types(self, nt_numeric_scalars : tuple):
        nt_type, py_value = nt_numeric_scalars

        nt_value = nt_type.create()
        nt_value['value'] = py_value
        assert bool(nt_value.value) == bool(py_value)
        assert int(nt_value.value) == int(py_value)
        assert float(nt_value.value) == pytest.approx(float(py_value))
        if isinstance(py_value, float):
            # pvxs .as_string() rounds to 4 digits
            assert str(nt_value.value) == str(round(py_value, 4))
        else:
            assert str(nt_value.value) == str(py_value)

    @pytest.mark.parametrize('nt_boolean_scalars', [
        (NTScalar(T.UInt32), 0),
        (NTScalar(T.Int32), 0),
        (NTScalar(T.Float32), 0),
        (NTScalar(T.Bool), False),
        (NTScalar(T.Bool), True),
    ], ids=[
        'uint32',
        'int32',
        'float32',
        'False',
        'True',
    ], indirect=False)
    def test_wrap_boolean_types(self, nt_boolean_scalars : tuple):
        nt_type, py_value = nt_boolean_scalars

        nt_value = nt_type.create()
        nt_value['value'] = py_value
        assert bool(nt_value.value) == bool(py_value)
        assert int(nt_value.value) == int(py_value)
        # pvxs returns "true/false" all lowercase
        assert str(nt_value.value) == str(py_value).lower()

    @pytest.mark.skip
    @pytest.mark.parametrize('nt_enum', [
        (NTEnum(), {'index': 2, 'choices': ['zero', 'one', 'two', 'three']}),
        #(NTEnum(), 2),
        #(NTEnum(), 'two'),
    ], ids=[
        'dict',
        #'index',
        #'label',
    ], indirect=False)
    def test_wrap_enum_type(self, nt_enum : tuple):
        nt_type, py_value = nt_enum

        nt_value = nt_type.create()
        nt_value['value'] = py_value
        _log.info("%s", repr(nt_value['value']))
        assert nt_value.value.index == py_value['index']
        assert nt_value.value.choices == py_value['choices']

    @pytest.mark.parametrize('nt_numeric_arrays', [
        (NTScalar(T.Int8A), [-2**7]*3),
        (NTScalar(T.Int16A), [-2**15]*3),
        (NTScalar(T.Int32A), [-2**31]*3),
        (NTScalar(T.Int64A), [-sys.maxsize]*3),
    ], ids=[
        'int8',
        'int16',
        'int32',
        'int64',
    ], indirect=False)
    def test_wrap_array_types(self, nt_numeric_arrays : tuple):
        nt_type, py_value = nt_numeric_arrays

        nt_value = nt_type.create()
        _log.info("PYTHON VALUE GOING IN %s", py_value)
        nt_value['value'] = py_value
        _log.info("NT VALUE THAT WAS SET %s", repr(nt_value))
        assert isinstance(nt_value.value, Value)
        assert nt_value.value.as_int_list() == py_value
