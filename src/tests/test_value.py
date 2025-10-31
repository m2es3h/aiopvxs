import array
import logging

import pytest

from aiopvxs.data import Member as M
from aiopvxs.data import TypeCodeEnum as T
from aiopvxs.data import TypeDef, Value
from aiopvxs.nt import NTEnum, NTScalar

_log = logging.getLogger(__file__)


class TestScalar:

    def test_wrap_integer_types(self, nt_integer_scalars : tuple):
        nt_type, _, py_value = nt_integer_scalars

        nt_value = nt_type.create()
        nt_value['value'] = py_value
        assert bool(nt_value.value) == bool(py_value)
        assert int(nt_value.value) == int(py_value)
        assert float(nt_value.value) == pytest.approx(float(py_value))

    def test_wrap_float_types(self, nt_float_scalars : tuple):
        nt_type, _, py_value = nt_float_scalars

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

    def test_wrap_string_type(self):
        test_string = "Hello, 👋"

        nt_value = NTScalar(T.String).create()
        nt_value['value'] = test_string
        assert str(nt_value.value) == test_string

    @pytest.mark.parametrize('nt_boolean_scalars', [
        (NTScalar(T.Bool), False),
        (NTScalar(T.Bool), True),
    ], ids=[
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

    #@pytest.mark.skip
    @pytest.mark.parametrize('nt_enum', [
        (NTEnum(), {'value.index': 2, 'value.choices': ['zero', 'one', 'two', 'three']}),
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
        assert nt_value.value.index.as_int() == py_value['value.index']
        assert nt_value.value.choices.as_string_list() == py_value['value.choices']

    def test_wrap_array_of_ints(self, nt_integer_arrays : tuple):
        nt_type, pyarray_type, py_value = nt_integer_arrays

        nt_value = nt_type.create()
        nt_value['value'] = array.array(pyarray_type, py_value)
        assert isinstance(nt_value.value, Value)
        assert nt_value.value.as_list() == py_value
        assert nt_value.value.as_int_list() == py_value
        assert nt_value.value.as_float_list() == [float(x) for x in py_value]

    def test_wrap_sequence_of_ints(self, nt_integer_arrays : tuple):
        nt_type, _, py_value = nt_integer_arrays

        nt_value = nt_type.create()
        nt_value['value'] = list(py_value)
        assert isinstance(nt_value.value, Value)
        assert nt_value.value.as_list() == py_value
        assert nt_value.value.as_int_list() == py_value
        assert nt_value.value.as_float_list() == [float(x) for x in py_value]

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

    def test_wrap_sequence_of_floats(self, nt_float_arrays : tuple):
        nt_type, pyarray_type, py_value = nt_float_arrays

        nt_value = nt_type.create()
        nt_value['value'] = list(py_value)
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

    def test_wrap_sequence_of_strings(self):
        test_strings = ["Hello, 👋", "from", "the", "python", "side"]

        nt_value = NTScalar(T.StringA).create()
        nt_value['value'] = test_strings
        print(repr(nt_value['value']))
        assert nt_value.value.as_list() == test_strings
        assert nt_value.value.as_string_list() == test_strings
