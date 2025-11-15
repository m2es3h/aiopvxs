/*
 * Project: aiopvxs
 * File:    pvxs_types.h
 *
 * This file is part of aiopvxs.
 *
 * https://github.com/m2es3h/aiopvxs
 *
 * Copyright (C) Michael Smith. All rights reserved.
 *
 * aiopvxs is free software: you can redistribute it and/or modify it
 * under the terms of The 3-Clause BSD License.
 *
 * https://opensource.org/license/bsd-3-clause
 *
 * aiopvxs is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/data.h>

namespace py = pybind11;
using namespace pvxs;

template <typename T>
bool make_shared_array(const py::buffer_info& info, shared_array<const void>& sa) {
    auto arr_begin = static_cast<const T*>(info.ptr);
    auto arr_end = static_cast<const T*>(arr_begin + info.shape[0]);
    sa = shared_array<const T>(arr_begin, arr_end).template castTo<const void>();
    return true;
}

template <typename T>
py::object make_python_array(const shared_array<const void>& sa) {
    py::object array_array = py::module_::import("array").attr("array");
    auto mv = py::memoryview::from_buffer(
        sa.data(),
        sizeof(T),
        py::format_descriptor<T>::value,
        {sa.size()},
        {sizeof(T)}
    );

    return array_array(py::format_descriptor<T>::format(), mv);
}

template <typename T>
bool load_from_python_seq(const py::sequence src, shared_array<const void>& sa) {
    try {
        auto seq_as_vector = src.cast<std::vector<T>>();
        shared_array<const T> new_value(seq_as_vector.begin(), seq_as_vector.end());
        sa = new_value.template castTo<const void>();
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool load_from_python_array(const py::buffer src, shared_array<const void>& sa) {
    py::buffer_info info = src.request();

    // veify buffer is 1D
    if (info.ndim != 1)
        return false;
    else if (info.item_type_is_equivalent_to<uint8_t>())
        return make_shared_array<uint8_t>(info, sa);
    else if (info.item_type_is_equivalent_to<uint16_t>())
        return make_shared_array<uint16_t>(info, sa);
    else if (info.item_type_is_equivalent_to<uint32_t>())
        return make_shared_array<uint32_t>(info, sa);
    else if (info.item_type_is_equivalent_to<uint64_t>())
        return make_shared_array<uint64_t>(info, sa);
    else if (info.item_type_is_equivalent_to<int8_t>())
        return make_shared_array<int8_t>(info, sa);
    else if (info.item_type_is_equivalent_to<int16_t>())
        return make_shared_array<int16_t>(info, sa);
    else if (info.item_type_is_equivalent_to<int32_t>())
        return make_shared_array<int32_t>(info, sa);
    else if (info.item_type_is_equivalent_to<int64_t>())
        return make_shared_array<int64_t>(info, sa);
    else if (info.item_type_is_equivalent_to<float>())
        return make_shared_array<float>(info, sa);
    else if (info.item_type_is_equivalent_to<double>())
        return make_shared_array<double>(info, sa);
    else
        throw std::runtime_error("Conversion not yet implemented.");
}

py::object cast_to_python_array(const shared_array<const void>& sa) {
    py::object array_array = py::module_::import("array").attr("array");

    // return python array with copy of shared_array contents
    switch (sa.original_type()) {
        case ArrayType::Bool:
        case ArrayType::UInt8:
            return make_python_array<uint8_t>(sa);
        case ArrayType::UInt16:
            return make_python_array<uint16_t>(sa);
        case ArrayType::UInt32:
            return make_python_array<uint32_t>(sa);
        case ArrayType::UInt64:
            return make_python_array<uint64_t>(sa);
        case ArrayType::Int8:
            return make_python_array<int8_t>(sa);
        case ArrayType::Int16:
            return make_python_array<int16_t>(sa);
        case ArrayType::Int32:
            return make_python_array<int32_t>(sa);
        case ArrayType::Int64:
            return make_python_array<int64_t>(sa);
        case ArrayType::Float32:
            return make_python_array<float>(sa);
        case ArrayType::Float64:
            return make_python_array<double>(sa);
        default:
            throw std::runtime_error("Cast not yet implemented.");
    }
}

namespace pybind11 {
namespace detail {

template <>
struct type_caster<shared_array<const void>> {
    PYBIND11_TYPE_CASTER(shared_array<const void>, io_name("collections.abc.Buffer | collections.abc.Sequence",
                                                           "aiopvxs.data.Value"));

    static handle
    cast(const shared_array<const void>& sa, return_value_policy policy, handle parent) {
        // return a list of strings if shared_array<std::string> type
        if (sa.original_type() == ArrayType::String) {
            py::list py_str_list;
            for (auto& item : sa.castTo<const std::string>())
                py_str_list.append(item);
            return py_str_list.release();
        }
        // inspect original data type in shared_array and copy-construct new python array
        else {
            return cast_to_python_array(sa).release();
        }
    }

    bool load(handle src, bool convert) {
        // check if py_object is a buffer
        if (py::isinstance<py::buffer>(src)) {
            // inspect data type of python array and copy-construct new shared_array
            py::buffer src_buffer = py::reinterpret_borrow<py::buffer>(src);
            return load_from_python_array(src_buffer, value);
        }
        // check if py_object is a sequence
        else if (py::isinstance<py::sequence>(src)) {
            // cast sequence to equivalent std::vector type and copy-construct new shared_array
            py::sequence seq = py::reinterpret_borrow<py::sequence>(src);
            if (py::isinstance<py::int_>(seq[0]))
                return load_from_python_seq<int64_t>(seq, value);
            else if (py::isinstance<py::float_>(seq[0]))
                return load_from_python_seq<double>(seq, value);
            else if (py::isinstance<py::str>(seq[0]))
                return load_from_python_seq<std::string>(seq, value);
            else
                return false;
        }
        else {
            return false;
        }
    }
};

} // namespace detail
} // namespace pybind11
