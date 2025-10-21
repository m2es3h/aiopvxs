import array
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
        assert nt_value.value.index == py_value['index']
        assert nt_value.value.choices == py_value['choices']

    @pytest.mark.parametrize('nt_integer_arrays', [
        (NTScalar(T.UInt8A),  'B', [2**7]*3),
        (NTScalar(T.UInt16A), 'H', [2**15]*3),
        (NTScalar(T.UInt32A), 'I', [2**31]*3),
        (NTScalar(T.UInt64A), 'Q', [sys.maxsize]*3),
        (NTScalar(T.Int8A),   'b', [-2**7]*3),
        (NTScalar(T.Int16A),  'h', [-2**15]*3),
        (NTScalar(T.Int32A),  'i', [-2**31]*3),
        (NTScalar(T.Int64A),  'q', [-sys.maxsize]*3),
    ], ids=[
        'uint8',
        'uint16',
        'uint32',
        'uint64',
        'int8',
        'int16',
        'int32',
        'int64',
    ], indirect=False)
    def test_wrap_array_of_ints(self, nt_integer_arrays : tuple):
        nt_type, pyarray_type, py_value = nt_integer_arrays

        nt_value = nt_type.create()
        nt_value['value'] = array.array(pyarray_type, py_value)
        assert isinstance(nt_value.value, Value)
        assert nt_value.value.as_list() == py_value
        assert nt_value.value.as_int_list() == py_value
        assert nt_value.value.as_float_list() == [float(x) for x in py_value]

    @pytest.mark.parametrize('nt_float_arrays', [
        (NTScalar(T.Float32A), 'f', [-42.24111557]*3),
        (NTScalar(T.Float64A), 'd', [-42.24111000167]*3),
    ], ids=[
        'float',
        'double',
    ], indirect=False)
    def test_wrap_array_of_floats(self, nt_float_arrays : tuple):
        nt_type, pyarray_type, py_value = nt_float_arrays

        nt_value = nt_type.create()
        nt_value['value'] = array.array(pyarray_type, py_value)
        assert isinstance(nt_value.value, Value)
        if pyarray_type == 'f':
            rounded_nt_vals = [round(v, 8) for v in nt_value.value.as_list()]
            assert rounded_nt_vals == py_value
            rounded_nt_vals = [round(v, 8) for v in nt_value.value.as_float_list()]
            assert rounded_nt_vals == py_value
        else:
            assert nt_value.value.as_list() == py_value
            assert nt_value.value.as_float_list() == py_value

        with pytest.raises(TypeError, match="'float' object cannot be interpreted as an integer"):
            assert nt_value.value.as_int_list() == [int(x) for x in py_value]
