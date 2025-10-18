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

bool load_from_python_array(const py::buffer src, shared_array<const void>& sa) {
    py::buffer_info info = src.request();

    // veify buffer is 1D
    if (info.ndim != 1) {
        return false;
    }
    else if (info.item_type_is_equivalent_to<uint8_t>()) {
        return make_shared_array<uint8_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<uint16_t>()) {
        return make_shared_array<uint16_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<uint32_t>()) {
        return make_shared_array<uint32_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<uint64_t>()) {
        return make_shared_array<uint64_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<int8_t>()) {
        return make_shared_array<int8_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<int16_t>()) {
        return make_shared_array<int16_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<int32_t>()) {
        return make_shared_array<int32_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<int64_t>()) {
        return make_shared_array<int64_t>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<float>()) {
        return make_shared_array<float>(info, sa);
    }
    else if (info.item_type_is_equivalent_to<double>()) {
        return make_shared_array<double>(info, sa);
    }
}

py::object cast_to_python_array(const shared_array<const void>& sa) {
    py::object array_array = py::module_::import("array").attr("array");

    /* return python array with copy of shared_array contents */
    switch (sa.original_type()) {
        case ArrayType::Bool:
        case ArrayType::UInt8:
            return array_array("B", sa.castTo<const uint8_t>());
        case ArrayType::UInt16:
            return array_array("H", sa.castTo<const uint16_t>());
        case ArrayType::UInt32:
            return array_array("I", sa.castTo<const uint32_t>());
        case ArrayType::UInt64:
            return array_array("Q", sa.castTo<const uint64_t>());
        case ArrayType::Int8:
            return array_array("b", sa.castTo<const int8_t>());
        case ArrayType::Int16:
            return array_array("h", sa.castTo<const int16_t>());
        case ArrayType::Int32:
            return array_array("i", sa.castTo<const int32_t>());
        case ArrayType::Int64:
            return array_array("q", sa.castTo<const int64_t>());
        case ArrayType::Float32:
            return array_array("f", sa.castTo<const float>());
        case ArrayType::Float64:
            return array_array("d", sa.castTo<const double>());
        //default:
        //    throw std::runtime_error("Conversion not yet implemented.");
    }
}

namespace pybind11 {
namespace detail {

#define SHARED_ARRAY_TYPE_CASTER(T, py_hint)                                                \
template <>                                                                                 \
struct type_caster<shared_array<const T>> {                                                 \
    PYBIND11_TYPE_CASTER(shared_array<const T>, io_name(py_hint, "aiopvxs.data.Value"));    \
                                                                                            \
    static handle                                                                           \
    cast(const shared_array<const T>& arr, return_value_policy policy, handle parent) {     \
        /* build a python list from shared_array<T> elements */                             \
        py::list py_list;                                                                   \
        for (auto val : arr) {                                                              \
            py_list.append(val);                                                            \
        }                                                                                   \
        return py_list.release();                                                           \
    }                                                                                       \
                                                                                            \
    bool load(handle src, bool convert) {                                                   \
        /* check if py_object is a buffer */                                                \
        if (py::isinstance<py::buffer>(src)) {                                              \
            py::buffer_info info = py::reinterpret_borrow<py::buffer>(src).request();       \
            /* verify buffer is 1D */                                                       \
            if (info.ndim != 1) {                                                           \
                return false;                                                               \
            }                                                                               \
            /* check this load() method matches buffer data type */                         \
            else if (!info.item_type_is_equivalent_to<T>()) {                               \
                return false;                                                               \
            }                                                                               \
            /* create new shared_array<T> and update value */                               \
            auto arr_begin = static_cast<const T*>(info.ptr);                               \
            auto arr_end = static_cast<const T*>(arr_begin + info.shape[0]);                \
            value = shared_array<const T>(arr_begin, arr_end);                              \
            return true;                                                                    \
        }                                                                                   \
        /* check if py_object is a sequence */                                              \
        else if (py::isinstance<py::sequence>(src)) {                                       \
            /* sequences not yet supported */                                               \
            return false;                                                                   \
        }                                                                                   \
        else {                                                                              \
            return false;                                                                   \
        }                                                                                   \
    }                                                                                       \
};

SHARED_ARRAY_TYPE_CASTER(int8_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(int16_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(int32_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(int64_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint8_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint16_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint32_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint64_t, "collections.abc.Buffer");

SHARED_ARRAY_TYPE_CASTER(float, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(double, "collections.abc.Buffer");


template <>
struct type_caster<shared_array<const void>> {
    PYBIND11_TYPE_CASTER(shared_array<const void>, io_name("collections.abc.Buffer", "aiopvxs.data.Value"));

    static handle
    cast(const shared_array<const void>& sa, return_value_policy policy, handle parent) {
        // inspect original data type in shared_array and copy-construct new python array
        return cast_to_python_array(sa).release();
    }

    bool load(handle src, bool convert) {
        /* check if py_object is a buffer */
        if (py::isinstance<py::buffer>(src)) {
            // inspect data type of python array and copy-construct new shared_array
            py::buffer src_buffer = py::reinterpret_borrow<py::buffer>(src);
            return load_from_python_array(src_buffer, value);
        }
        /* check if py_object is a sequence */
        else if (py::isinstance<py::sequence>(src)) {
            /* sequences not yet supported */
            return false;
        }
        else {
            return false;
        }
    }
};

} // namespace detail
} // namespace pybind11
